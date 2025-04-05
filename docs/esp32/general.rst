.. _esp32_general:

ESP32基本信息
========================================

ESP32是有乐鑫科技开发的主流的嵌入式WiFi蓝牙SoC（System-on-Chip）。

多样的开发板
-------------------

现在许多来自不同生产厂商（包括乐鑫）的开发板搭载ESP32芯片。MicroPython试图提供一个通用端口，
该端口可以在尽可能多的开发板或模块上运行，但可能存在一些限制。乐鑫开发板作为端口的参考
（例如，对它们进行测试）。对于您正在使用的任何电路板，请确保您有数据表、示意图和其他参考
资料，以便您可以查找任何电路板的特定功能。

为了制作一个通用的ESP32端口并支持尽可能多的板，做出了以下设计和实施决定：

* GPIO管脚编号基于ESP32芯片编号。请将您的电路板的手册或管脚图放在手边，以便找到您的电路板管脚
  和实际的ESP32管脚之间的对应关系。
* MicroPython支持所有管脚，但并非所有管脚都可以在任何板上使用。
  例如，不应使用连接到外部SPI闪存的管脚，并且电路板可能只公开某些管脚选择。


技术参数和SoC数据表
-------------------------------------------

ESP32芯片的数据表和其他参考资料可从供应商网站获得：
https://www.espressif.com/en/support/download/documents?keys=esp32
这里有芯片技术规格、性能、工作模式、内部功能等的主要参考。

为了方便你了解ESP32，我们给你提供了部分参数：

* 架构: Xtensa Dual-Core 32-bit LX6
* CPU频率: up to 240MHz
* RAM可用空间: 528KB (保留一部分给系统)
* BootROM: 448KB
* 内部FlashROM: none
* 外部FlashROM: code and data, via SPI Flash; usual size 4MB
* GPIO: 34 (gpio与其他功能多路复用，包括外部FlashROM、UART等。)
* UART: 3个RX/TX UART (无硬件握手), 一个TX-only UART
* SPI: 4 SPI interfaces (one used for FlashROM)
* I2C: 2 I2C (任何管脚上都有bitbang实现)
* I2S: 2
* ADC: 12位 SAR ADC 最高18频道
* DAC: 2个8位 DACs
* RMT: 8通道允许精确的脉冲发射/接收
* 编程方式: 从UART使用BootROM bootloader-由于外部FlashROM和始终可用的BootROM bootloader，ESP32是不可分块的


获取更多有关ESP32的信息？请访问：https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf

MicroPython是在ESP-IDF（乐鑫科技的ESP32开发框架）之上实现的。这是一个基于FreeRTOS的系统。
想要深入了解ESP-IDF？请移步至：
`ESP-IDF编程指南 <https://docs.espressif.com/projects/esp-idf/en/latest/index.html>`_
