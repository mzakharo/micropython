.. _columbus_quickref:

哥伦布 快速参考手册
===============================

以下是哥伦布描述图：

.. image:: http://www.01studio.org/micropython/picture/COLUMBUS.png
	:alt: PYBv1.1 pinout
	:width: 700px

以下是快速参考内容，如果你是第一次使用哥伦布开发板，请考虑先阅读以下章节内容：

.. toctree::
   :maxdepth: 1

   general.rst

通用控制
---------------------
See :mod:`pyb`. ::

    import pyb

    pyb.repl_uart(pyb.UART(1, 9600)) # 复制 REPL 到 UART(1)
    pyb.wfi() # 暂停 CPU，等待中断
    pyb.freq() # 获取 CPU 和总线的频率
    pyb.freq(60000000) # 设置 CPU 工作频率为 60MHz
    pyb.stop() # 暂停 CPU, 等待外部中断

延时和计时器
----------------

使用 :mod:`time <utime>` 模块::

    import time

    time.sleep(1)           # 睡眠1s
    time.sleep_ms(500)      # 睡眠500ms
    time.sleep_us(10)       # 睡眠10ms
    start = time.ticks_ms() # 获取毫秒计数器的值
    delta = time.ticks_diff(time.ticks_ms(), start) # 计算启动时间

板载LEDs
-------------

See :ref:`pyb.LED <pyb.LED>`. ::

    from pyb import LED

    led = LED(1) # 1=红, 2=绿, 3=黄, 4=蓝
    led.toggle()
    led.on()
    led.off()

板载按键
---------------

See :ref:`pyb.Switch <pyb.Switch>`. ::

    from pyb import Switch

    sw = Switch()
    sw.value() # 返回 True 或者 False
    sw.callback(lambda: pyb.LED(1).toggle()) #按键按下执行相关函数

引脚和GPIO口
-------------

See :ref:`pyb.Pin <pyb.Pin>`. ::

    from pyb import Pin

    p_out = Pin('A0', Pin.OUT_PP)
    p_out.high()
    p_out.low()

    p_in = Pin('E3', Pin.IN, Pin.PULL_UP)
    p_in.value() # 获取数值, 0 或者 1

舵机控制
-------------

See :ref:`pyb.Servo <pyb.Servo>`. ::

    from pyb import Servo

    s1 = Servo(1) # 舵机连接到接口1 (X1, VIN, GND)
    s1.angle(45) # 旋转到45°位置
    s1.angle(-60, 1500) # 在1500毫秒内转到-60°的位置
    s1.speed(50) # 适用于连续旋转舵机

注意: 哥伦布开发板只有3路舵机接口 1->A0, 2->A2, 3->A3。

外部中断
-------------------

See :ref:`pyb.ExtInt <pyb.ExtInt>`. ::

    from pyb import Pin, ExtInt

    callback = lambda e: print("intr")
    ext = ExtInt(Pin('A0'), ExtInt.IRQ_RISING, Pin.PULL_NONE, callback)

计时器
------

See :ref:`pyb.Timer <pyb.Timer>`. ::

    from pyb import Timer

    tim = Timer(1, freq=1000)
    tim.counter() # 获取计时器数值
    tim.freq(0.5) # 0.5 Hz
    tim.callback(lambda t: pyb.LED(1).toggle())

实时时钟
---------------------

See :ref:`pyb.RTC <pyb.RTC>` ::

    from pyb import RTC

    rtc = RTC()
    rtc.datetime((2017, 8, 23, 1, 12, 48, 0, 0)) # 设置日期和时间
    rtc.datetime() # 获取日期和时间

PWM (脉宽调变) 
----------------------------

See :ref:`pyb.Pin <pyb.Pin>` and :ref:`pyb.Timer <pyb.Timer>`. ::

    from pyb import Pin, Timer

    p = Pin('A0') # X1 has TIM2, CH1
    tim = Timer(2, freq=1000)
    ch = tim.channel(1, Timer.PWM, pin=p)
    ch.pulse_width_percent(50)

ADC (模数转换) 
----------------------------------

See :ref:`pyb.Pin <pyb.Pin>` and :ref:`pyb.ADC <pyb.ADC>`. ::

    from pyb import Pin, ADC

    adc = ADC(Pin('A5'))
    adc.read() # 读取数值, 0-4095

DAC (数模转换) 
----------------------------------

See :ref:`pyb.Pin <pyb.Pin>` and :ref:`pyb.DAC <pyb.DAC>`. ::

    from pyb import Pin, DAC

    dac = DAC(Pin('A5'))
    dac.write(120) # 输出数值 0 至 255

UART(串行总线) 
-----------------

See :ref:`pyb.UART <pyb.UART>`. ::

    from pyb import UART

    uart = UART(3, 115200)
    uart.write('hello')
    uart.read(5) # 读取 5 个字节

SPI总线
-------

See :ref:`pyb.SPI <pyb.SPI>`. ::

    from pyb import SPI

    spi = SPI(1, SPI.MASTER, baudrate=200000, polarity=1, phase=0)
    spi.send('hello')
    spi.recv(5) # 接收5个字节
    spi.send_recv('hello') # 发送和接收5个字节

I2C总线
-------

硬件I2C可以直接通过X和Y半部分接口直接定义成 ``I2C('X')`` 和 ``I2C('Y')`` ，此时使用默认的I2C接口。
另外也可以直接使用标识符，例如 ``I2C(1)``。  软件I2C可以通过 ``scl`` 和 ``sda`` 结合引脚来自定义。
更多详细信息请看 :ref:`machine.I2C <machine.I2C>`. ::

    from machine import I2C

    i2c = I2C('X', freq=400000)                 # 定义硬件I2C对象
    i2c = I2C(scl='B10', sda='B11', freq=100000)  # 定义软件I2C对象

    i2c.scan()                          # 返回扫描到的从机地址
    i2c.writeto(0x42, 'hello')          # 往地址为 0x42 的从机写5个字节
    i2c.readfrom(0x42, 5)               # 从地址为 0x42 的从机读取5个字节

    i2c.readfrom_mem(0x42, 0x10, 2)     # 从设备地址 0x42和存储器地址为0x10中读取2个字节
    i2c.writeto_mem(0x42, 0x10, 'xy')   # 从设备地址 0x42和存储器地址为0x10中写入2个字节

注意: 对于传统的I2C支持，请参阅 :ref:`pyb.I2C <pyb.I2C>`.

CAN总线 (区域网络控制)
---------------------------------

See :ref:`pyb.CAN <pyb.CAN>`. ::

    from pyb import CAN

    can = CAN(1, CAN.LOOPBACK)
    can.setfilter(0, CAN.LIST16, 0, (123, 124, 125, 126))
    can.send('message!', 123)   # 发送ID为123的消息
    can.recv(0)                 # 在 FIFO 0 上接收信息
