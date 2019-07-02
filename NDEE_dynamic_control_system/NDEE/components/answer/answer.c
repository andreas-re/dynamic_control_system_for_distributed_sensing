#include "esp_log.h"
#include "answer.h"
#include "string.h"
#include "math.h"
#include "assert.h"
#include "esp_heap_caps.h"
#include "multi_heap.h"
#include <stdio.h>
//#include "esp_log.h"

static const char *TAG = "ANSWER";

int exponent;

answer_decoded_t *answer_decoded_new()
{
    answer_decoded_t *answer_decoded = malloc(sizeof(*answer_decoded));
    return answer_decoded;
}

char *measurand_to_bitstring(char *measurand)
{
    if (strcmp(measurand, "TEMPERATURE") == 0) {
        return "00000000";
    }
    if (strcmp(measurand, "CO2") == 0) {
        return "00000001";
    }
    if (strcmp(measurand, "HUMIDITY") == 0) {
        return "00000010";
    }
    if (strcmp(measurand, "AIRPRESSURE") == 0) {
        return "00000011";
    }
    return "";
}

char *unit_to_bitstring(char *unit)
{
    if (strcmp(unit, "DEGC") == 0) {
        return "00000000";
    }
    if (strcmp(unit, "PPM") == 0) {
        return "00000001";
    }
    if (strcmp(unit, "GM3") == 0) {
        return "00000010";
    }
    if (strcmp(unit, "BAR") == 0) {
        return "00000011";
    }
    return "";
}

char *device_to_bitstring(char *device)
{
    if (strcmp(device, "BMP180") == 0) {
        return "00000000";
    }
    if (strcmp(device, "GASSENSOR") == 0) {
        return "00000001";
    }
    if (strcmp(device, "HUMIDITYSENSOR0") == 0) {
        return "00000010";
    }
    return "";
}

char *type_to_bitstring(char *type)
{
    if (strcmp(type, "INTEGER") == 0) {
        return "00000000";
    }
    if (strcmp(type, "FLOAT") == 0) {
        return "00000001";
    }
    return "";
}

char* append(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    free(s1);
    return result;
}

char *integer_to_bitstring_helper (int number, char *result, int i, int leading)
{
    if (i < 0){
        return result;
    }
    int divisor = pow(2, i);
    int temp = number / divisor;
    char *res;
    if (temp == 1){
        res = "1";
    }
    else if (leading){
        res = "0";
    }
    else if (strcmp(result, "") != 0){
        res = "0";
    }
    else if (i == 0){
        res = "0";
    }
    else {
        res = "";
    }
    result = append (result, res);
    number = number - temp * divisor;
    i--;
    return integer_to_bitstring_helper(number, result, i, leading);
}

char *integer_sign_helper (int number, char *result, int i, int leading)
{
    if (number >= 0) {
        if (leading) {
            result = append (result, "0");
        }
        return integer_to_bitstring_helper (number, result, i-1, leading);
    }
    result = append (result, "1");
    number = pow(2, i) + number;
    return integer_to_bitstring_helper (number, result, i-1, false);
}

char *integer_to_signed_bitstring (int number, int digits, int leading)
{
    char *result;
    result = malloc(sizeof("") + 1);
    result = strcpy(result, "");
    return integer_sign_helper (number, result, digits-1, leading);
}

char *integer_to_unsigned_bitstring(int number, int digits, int leading)
{
    char *result;
    result = malloc(sizeof("") + 1);
    result = strcpy(result, "");
    return integer_to_bitstring_helper (number, result, digits-1, leading);
}

char *float_sign_to_bit (float number)
{
    char *result;
    result = malloc(sizeof("1") + 1);
    if (number < 0){
        strcpy(result, "1");
        return result;
    }
    strcpy(result, "0");
    return result;
}

char* float_significant_to_bit_helper (float number, char *result)
{
    int digits;
    digits = 1;
    if (strlen(result) == 23){
        return result;
    }
    if (strlen(result) == 0){
        digits = 23;
    }
    char *temp;
    temp = integer_to_unsigned_bitstring(number, digits, false);
    result = append(result, temp);
    free(temp);
    if (digits == 23){
        exponent = strlen(result) -1;
    }
    number = number - (int) number;
    number = number * 2;
    return float_significant_to_bit_helper(number, result);
}

char* float_significant_to_bit (float number)
{
    if (number < 0) {
        number = number * (-1);
    }
    char *input;
    char *result;
    int i;
    int len;
    input = malloc(sizeof("") + 1);
    strcpy(input, "");
    result = float_significant_to_bit_helper (number, input);
    len=strlen(result);
    for(i=0;i<len-1;i++)
    {
        result[i]=result[i+1];
    }
    result[i]='0';
    return result;
}

char* float_exponent_to_bit ()
{
    char *result;
    int temp;
    temp = exponent + 127;
    
    result = integer_to_unsigned_bitstring (temp, 8, true);
    
    return result;
}

char* float_to_bitstring (float number)
{
    char *sign;
    char *exponent;
    char *significant;
    char *result;
    sign = float_sign_to_bit(number);
    significant = float_significant_to_bit (number);
    exponent = float_exponent_to_bit();
    result = append (sign, exponent);
    free(exponent);
    result = append (result, significant);
    free(significant);
    return result;
}

char* fusion(answer_encoded_t *answer_encoded)
{
    char *result;
    result = malloc(sizeof("") + 1);
    strcpy(result, "");
    result = append(result, (*answer_encoded).status);
    result = append(result, (*answer_encoded).requestid);
    result = append(result, (*answer_encoded).timestamp);
    result = append(result, (*answer_encoded).measurand);
    result = append(result, (*answer_encoded).device);
    result = append(result, (*answer_encoded).unit);
    result = append(result, (*answer_encoded).type);
    result = append(result, (*answer_encoded).value);

    /*ESP_LOGI(TAG, "Answer: ");
    ESP_LOGI(TAG, "status: %s", (*answer_encoded).status);
    ESP_LOGI(TAG, "requestid: %s", (*answer_encoded).requestid);
    ESP_LOGI(TAG, "timestamp: %s", (*answer_encoded).timestamp);
    ESP_LOGI(TAG, "measurand: %s", (*answer_encoded).measurand);
    ESP_LOGI(TAG, "device: %s", (*answer_encoded).device);
    ESP_LOGI(TAG, "unit: %s", (*answer_encoded).unit);
    ESP_LOGI(TAG, "type: %s", (*answer_encoded).type);
    ESP_LOGI(TAG, "value: %s", (*answer_encoded).value);*/

    free(answer_encoded);
    return result;
}

char* value_to_bitstring(float value, char *type)
{
    if (strcmp (type, "INTEGER") == 0){
        return integer_to_signed_bitstring(value, 32, true);
    }
    if (strcmp (type, "FLOAT") == 0){
        return float_to_bitstring(value);
    }
        return "";
}

char *encode(answer_decoded_t *answer_decoded)
{
    char *type;
    type = (*answer_decoded).type;
    answer_encoded_t *answer_encoded = malloc(sizeof(*answer_encoded));
    (*answer_encoded).status = integer_to_signed_bitstring((*answer_decoded).status, 8, true);
    (*answer_encoded).requestid = integer_to_signed_bitstring((*answer_decoded).requestid, 8, true);
    (*answer_encoded).timestamp = integer_to_signed_bitstring((*answer_decoded).timestamp, 32, true);
    (*answer_encoded).measurand = measurand_to_bitstring((*answer_decoded).measurand);
    (*answer_encoded).device = device_to_bitstring((*answer_decoded).device);
    (*answer_encoded).unit = unit_to_bitstring((*answer_decoded).unit);
    (*answer_encoded).type = type_to_bitstring(type);
    (*answer_encoded).value = value_to_bitstring((*answer_decoded).value, type);
    char *result;
    result = fusion(answer_encoded);
    return result;
}
