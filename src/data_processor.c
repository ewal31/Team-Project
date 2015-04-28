#include <stdint.h>

#include "arm_math.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "controller.h"
#include "rfm73.h"
#include "sd_card.h"

/* Current user inputs. */
static int wheelSize = 600; // mm 
static int carWeight = 1000; // kg

/* Current values for all the metrics. */
static int currentSpeed; // Stored with no fractional part. (km/h)
static int distance; // Stored with 2 decimals. (km)
static int lateralAcceleration; // Stored with no fractional part. (m/s/s)
static int currentPower; // Stored with no fractional part. 

/* For doing actual calculations. */
static float lastSpeed;
static float distAccurate; 

extern QueueHandle_t dataQueue;

#define DATA_SAMPLE_TIME_MAX 110
#define DATA_SAMPLE_RATE 10.f
#define MAX_SAMPLES_MISSED 300
#define NEW_DATA_SAMPLE_DELAY 5

//float32_t accelData[75] = {
//    13.983925f, 11.146716f, 8.020234f, 5.076196f, 2.209518f, -0.381647f, -2.263066f, -3.396946f, 
//    -3.375577f, -2.579166f, -0.548134f, 2.199800f, 5.647203f, 9.142277f, 12.442171f, 15.460230f, 
//    17.444640f, 18.386287f, 18.219408f, 16.561719f, 13.896006f, 10.437010f, 6.463821f, 2.694632f, 
//    -0.496924f, -2.601744f, -3.446053f, -2.598584f, -0.517271f, 2.882228f, 7.178827f, 11.191550f,
//    14.938665f, 17.594172f, 18.505405f, 17.839560f, 15.314147f, 11.429056f, 6.808807f, 2.498946f, 
//    -1.277613f, -3.162069f, -3.201192f, -1.336754f, 2.357491f, 7.143585f, 11.849569f, 15.786234f, 
//    18.175257f, 18.327274f, 16.060159f, 12.183908f, 7.326511f, 2.206321f, -1.576392f, -3.530895f, 
//    -2.708014f, 0.461482f, 5.092877f, 10.673821f, 15.298680f, 17.987696f, 18.189695f, 15.376992f, 
//    10.613097f, 5.059314f, 0.013003f, -3.100070f, -3.388026f, -0.433783f, 4.398677f, 10.464247f, 
//    15.606272f, 18.270471f, 17.827490f, 
//};

/* Circular buffer for past samples. */
#define SAMPLES_STORED 75
static float32_t accelData[SAMPLES_STORED];
static int pos;

/* FFT variables. */
#define FFT_SIZE 1024U
#define FFT_DC_SKIP 30U
static arm_rfft_fast_instance_f32 fftInstance;
static float32_t input[FFT_SIZE];
static float32_t output[FFT_SIZE];

/* Radio Control */
static int radioPresent;
static int sampling;
static int samplesMissed = 50;
static uint8_t buff[32];

static float start_speed_calculation(void);
static void init_data_processing(void);
static void reset_readings(void);
static void send_current_data(void);

void process_new_input(uint8_t *data, int doCalculations);

void processing_task(void *params)
{
    TickType_t lastContact = 0, currentTime;
    int status, samplesDelayed = 0;

    vTaskDelay(100);
    rfm73_init();
    init_data_processing();

    while (1) {
        currentTime = xTaskGetTickCount();
        status = rfm73_check_fifo(); 
        if (status) {
            rfm73_receive_packet(buff);
            if (samplesMissed > 2) {
                samplesDelayed = NEW_DATA_SAMPLE_DELAY;
            }
            samplesMissed = 0;

            /* Only update the values after we have re-built enough of the
             * data.
             */
            if (samplesDelayed) {
                --samplesDelayed;
                process_new_input(buff, 0);
            } else {
                process_new_input(buff, 1);
            }

            if (sampling) {
                send_current_data();
            }

            lastContact = currentTime;
        } else {
            if (currentTime - lastContact > DATA_SAMPLE_TIME_MAX) {
//                rfm73_receive_packet(buff);
//                rfm73_flush_fifo();
                /* We missed a sample. */
                lastContact = currentTime - 10;
                samplesMissed++;
                if (samplesMissed < 3) {
                    /* If this is our first missed sample, pad with 0's. This
                     * should only cause a minor discontinutiy.
                     */
                    memset(buff, 0, 32);
                    process_new_input(buff, 1);
                } else if (samplesMissed == 3) {
                    /* If we miss any more than one sample we're going to
                     * really mess with the FFT. We will 0 the readings and
                     * start rebuilding the data when we get signal back. We
                     * only need to do this once so only check == 2.
                     */
                    reset_readings();
                }

                if (sampling && samplesMissed > MAX_SAMPLES_MISSED) {
                    /* We should end the trip now if we are recording data. */
                    end_journey_request(0); 
                    sampling = 0;
                } else if (sampling) {
                    send_current_data();
                }
            }
        }

        radioPresent = (samplesMissed < 5 ? 1 : 0);
        
        vTaskDelay(5);
    }
}

void data_processor_end_recording(void)
{
    sampling = 0;
}

void data_processor_start_recording(void)
{
    sampling = 1;
}

static void send_current_data(void)
{
    struct car_data currentData;

    currentData.speed = currentSpeed;
    currentData.distance = distance;
    currentData.accel = lateralAcceleration;
    currentData.power = currentPower;

    if (dataQueue) {
        xQueueSendToBack(dataQueue, &currentData, 50);
    }
}

/* Call this at the required sample rate or the data will get out of sync. */
void process_new_input(uint8_t *data, int doCalculations)
{
    int16_t x, z, zSum = 0;
    float newSpeed, averageSpeed, distDelta, lateralAccel;

    for (int i = 0; i < 5; ++i) {
        x = data[(6 * i) + 1] << 8 | data[6 * i];
        z = data[(6 *i) + 5] << 8 | data[(6 * i) + 4];
        zSum += z;
        accelData[pos] = (float)x;
        pos = (pos + 1) % SAMPLES_STORED;
    }
    
    if (!doCalculations) {
        return;
    }
    
    /* Average lateral acceleration over the last 5 samples. */
    zSum /= 5;

    newSpeed = start_speed_calculation();

    /* Poor mans interpolation. */
    distDelta = newSpeed / (DATA_SAMPLE_RATE * 3.6f);

    /* Scale the lateral accel to m/s. */
    lateralAccel = (float)zSum * 9.8f * 4.f / 512.f;

    lastSpeed = newSpeed;
    distAccurate += distDelta;

    currentSpeed = (int)newSpeed; // No decimal;
    distance = (int)(distAccurate / 10.f); // 2 Decimal places (interpret as km). 
    lateralAcceleration = (int)lateralAccel; // No decimal.
}

static float start_speed_calculation(void)
{
    int p = pos;
    float wheelDiam = (float)wheelSize / 1000.f;
    float max, spd, mean;
    uint32_t ind;

    /* First step - zero out the input array. */
    arm_fill_f32(0.f, input, FFT_SIZE);
    
    /* Copy the data to the array in time order. */
    for (int i = 0; i < SAMPLES_STORED; ++i) {
        input[i] = accelData[p];
        p = (p + 1) % SAMPLES_STORED;
    }

    /* Take the FFT of the new data. */
    arm_rfft_fast_f32(&fftInstance, input, output, 0);

    /* Throw away the second half of the DC component. This is actually the
     * real part for the second symmetric half, which we don't need. 
     */
    output[1] = 0.f;

    /* Calculate the magnitude of the complex values. */
    arm_cmplx_mag_f32(output, input, FFT_SIZE / 2);

    /* Find the max. Here I throw away the first 20 samples because the DC peak
     * is too high for us to deal with. May need some more sophisticated
     * detection here for low speeds. Just ignore it for now.
     */
    arm_max_f32(input + FFT_DC_SKIP, (FFT_SIZE / 2) - FFT_DC_SKIP, &max, &ind);

    /* Calcualte the mean value of the FFT, then determine if the max is
     * actually due to rotation or just a noise spike.
     */
    arm_mean_f32(input + 5, (FFT_SIZE / 2) - 5, &mean);

    if (max > (4.f * mean)) {
        /* index * 60 * 60 * diam * pi  * fs / (1000 * (fftlen/2 - 1)) */
        spd = (float)(ind + FFT_DC_SKIP) * 3.6f * PI * wheelDiam * 25 / 511.f;
    } else {
        /* Was probably just noise, look at the low frequency components. */
        arm_max_f32(input, FFT_DC_SKIP, &max, &ind);
        spd = (float)ind * 3.6f * PI * wheelDiam * 25 / 511.f;
    }

    return spd;
}

static void reset_data(void)
{
    arm_fill_f32(0.f, accelData, SAMPLES_STORED);
}

static void reset_readings(void)
{
    currentSpeed = 0; 
    lateralAcceleration = 0; 
    currentPower = 0; 
    lastSpeed = 0.f;

    arm_fill_f32(0.f, accelData, SAMPLES_STORED);
}

static void init_data_processing(void)
{
    arm_rfft_fast_init_f32(&fftInstance, FFT_SIZE);
//    arm_fill_f32(0.f, accelData, SAMPLES_STORED);
}

/* Functions for getting and setting the car properties and current metrics. */

void set_current_weight(int weight)
{
    carWeight = weight;
}

int get_current_weight(void)
{
    return carWeight;
}

void set_current_wheel_size(int size)
{
    wheelSize = size;
}

int get_current_wheel_size(void)
{
    return wheelSize;
}

int get_current_speed(void)
{
    return currentSpeed;
}

int get_current_distance(void)
{
    return distance;
}

int get_current_accel(void)
{
    return lateralAcceleration;
}

int get_current_power(void)
{
    return currentPower;
}

int get_radio_status(void)
{
    return radioPresent;
}
