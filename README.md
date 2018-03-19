# CIFAR10 Image extractor

A very simple C program to extract an image from CIFAR10 [1] binary file. The image is saved in three format:
1. an header file that can be used with the CMSIS Neural Network Library [2] "Convolutional Neural Network Example" example.
2. a RGB raw image file
3. a BMP image file.

The program must be called with the image binary file name as parameter, and optionally you can pass the number of the image to extract.
If you omit to specify the image number, the program generate a random number.

[1] https://www.cs.toronto.edu/~kriz/cifar.html
[2] http://www.keil.com/pack/doc/CMSIS_Dev/NN/html/index.html


