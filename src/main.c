#include <stdio.h>
#include "main.h"
void foo() {
    printf("foo\n");
}


void bmi_calculator() {
    float weight, height, bmi;
    printf("Enter your weight in kg: ");
    scanf("%f", &weight);
    printf("Enter your height in meters: ");
    scanf("%f", &height);
    bmi = weight / (height * height);
    printf("Your BMI is: %f\n", bmi);
    if (bmi < 18.5) {
        printf("You are underweight\n");
    } else if (bmi >= 18.5 && bmi < 25) {
        printf("You are normal\n");
    } else if (bmi >= 25 && bmi < 30) {
        printf("You are overweight\n");
    } else {
        printf("You are obese\n");
    }
}






int main() {
    printf("Hello, World!\n");
    printf("Hello, World!!!!!!!!!!!!!!\n");


    bmi_calculator();
    
    printf("Hello, World!!!!!!!!!!!!!!\n");

    foo();

    return 0;
}