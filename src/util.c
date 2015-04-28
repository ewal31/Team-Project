/* util.c
 *
 * Utility functions.
 *
 * Written for ENGG4810 Team 30 by:
 * - Mitchell Krome
 * - Edward Wall
 *
 */

/* Converts num to a string with fracNum decimal places. Returns how long the
 * written string is
 */
int write_dec_num(int num, int fracNum, char *buff)
{
    char tmp[20] = { 0 };
    int pos = 0;
    int neg = 0;

    if (num < 0) {
        neg = 1;
        num = num * -1;
    }

    if (fracNum) {
        for (int i = 0; i < fracNum; ++i) {
            tmp[pos++] = num % 10 + '0';
            num /= 10;
        }
        tmp[pos++] = '.';
    }

    if (!num) {
        tmp[pos++] = '0';
    }

    while (num) {
        tmp[pos++] = num % 10 + '0';
        num /= 10;
    }

    if (neg) {
        tmp[pos++] = '-';
    }

    for (int i = 0; i < pos; ++i) {
        buff[i] = tmp[pos - 1 - i];
    }

    buff[pos] = '\0';

    return pos;
}
