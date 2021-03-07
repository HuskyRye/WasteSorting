
#   Copyright(C) 2021 刘臣轩

#   This program is free software: you can redistribute it and / or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.

#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY without even the implied warranty of 
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program. If not, see <http://www.gnu.org/licenses/>.


#!/usr/bin/python
# -*-coding:utf-8-*-

from tflite_runtime.interpreter import Interpreter
from PIL import Image
import cv2
import re
import os
import numpy as np


def loadLabels(labelPath):
    p = re.compile(r'\s*(\d+)(.+)')
    with open(labelPath, 'r', encoding='utf-8') as labelFile:
        lines = (p.match(line).groups() for line in labelFile.readlines())
        return {int(num): text.strip() for num, text in lines}


def load_labels(path):
    with open(path, 'r', errors='ignore') as f:
        return {i: line.strip() for i, line in enumerate(f.readlines())}


def set_input_tensor(interpreter, image):
    tensor_index = interpreter.get_input_details()[0]['index']
    input_tensor = interpreter.tensor(tensor_index)()[0]
    input_tensor[:, :] = image


def classify_image(interpreter, image, top_k=1):
    set_input_tensor(interpreter, image)
    interpreter.invoke()
    output_details = interpreter.get_output_details()[0]
    output = np.squeeze(interpreter.get_tensor(output_details['index']))
    # If the model is quantized (uint8 data), then dequantize the results
    if output_details['dtype'] == np.uint8:
        scale, zero_point = output_details['quantization']
        output = scale * (output - zero_point)

    ordered = np.argpartition(-output, top_k)
    return [(i, output[i]) for i in ordered[:top_k]]


def main():
    # labels = load_labels('labels_mobilenet_quant_v1_224.txt')
    # interpreter = Interpreter('mobilenet_v1_1.0_224_quant.tflite')
    # labels = loadLabels('../WasteSorting/tensorflow/labels.txt')
    interpreter = Interpreter('../WasteSorting/tensorflow/model.tflite')
    interpreter.allocate_tensors()

    pil_im = Image.open('../WasteSorting/WasteSorting.jpg').convert(
        'RGB').resize((224, 224), Image.ANTIALIAS)
    pil_im.transpose(Image.FLIP_LEFT_RIGHT)

    results = classify_image(interpreter, pil_im)
    # print(results)
    label = results[0][0]
    if label == 0:
        print('识别失败')
    elif label in range(1, 4):
        print('有害垃圾')
    elif label in range(4, 7):
        print('可回收垃圾')
    elif label in range(7, 10):
        print('厨余垃圾')
    else:
        print('其他垃圾')


if __name__ == '__main__':
    main()
