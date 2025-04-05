# MicroPython port to the TKM32F499

## 一、简介

- [x] TKM32F499 是好钜润科技 自主研发的高性能32位M4内核MCU，主频高达240M。具有丰富外设，详细http://hjrkj.com/


- [x] micropython port tkm32由零一电子科技(01Studio) http://www.01studio.org，团队开发维护，好钜润科技提供技术支持。

## 二、模块支持情况

| 模块(Module) | 支持情况 |               备注                |
| :----------: | :------: | :-------------------------------: |
|     Pin      |   支持   |                                   |
|     UART     |   支持   | 数据位只支持8位模式，暂未支持流控 |
|     I2C      |   支持   |    暂时支持硬件 主机 I2C 模式     |
|   SoftI2C    |   支持   |              软件I2C              |
|     SPI      |   支持   |      硬件SPI暂时支持主机模式      |
|   SoftSPI    |   支持   |              软件SPI              |
|    Timer     |   支持   |          1-10个硬件Timer          |
|     RTC      |   支持   |                                   |
|     LCD      |   支持   |    支持RGB888， 7、4.3寸RGB屏     |
|              |          |                                   |
|              |          |                                   |
|              |          |                                   |
|              |          |                                   |
|              |          |                                   |
