.. _esp8266_quickref:

===============================




.. toctree::
   :maxdepth: 1

   general.rst
   tutorial/index.rst

----------------------


---------------------


The :mod:`machine` module::

    import machine


The :mod:`esp` module::

    import esp


Networking
----------

The :class:`network.WLAN` class in the :mod:`network` module::

    import network

    wlan = network.WLAN(network.WLAN.IF_STA) # create station interface
    wlan.active(True)       # activate the interface
    wlan.scan()             # scan for access points
    wlan.isconnected()      # check if the station is connected to an AP
    wlan.connect('ssid', 'key') # connect to an AP
    wlan.config('mac')      # get the interface's MAC address
    wlan.ipconfig('addr4')  # get the interface's IPv4 addresses

    ap = network.WLAN(network.WLAN.IF_AP) # create access-point interface
    ap.active(True)         # activate the interface
    ap.config(ssid='ESP-AP') # set the SSID of the access point

    def do_connect():
        import network
        wlan = network.WLAN(network.WLAN.IF_STA)
        wlan.active(True)
        if not wlan.isconnected():
            print('connecting to network...')
            wlan.connect('ssid', 'key')
            while not wlan.isconnected():
                pass
        print('network config:', wlan.ipconfig('addr4'))



Use the :mod:`time <time>` module::

    import time


------


    from machine import Timer



-------------


    from machine import Pin






There's a higher-level abstraction :ref:`machine.Signal <machine.Signal>`
which can be used to invert a pin. Useful for illuminating active-low LEDs
using ``on()`` or ``value(1)``.

UART (serial bus)
-----------------

See :ref:`machine.UART <machine.UART>`. ::

    from machine import UART
    uart = UART(0, baudrate=9600)
    uart.write('hello')
    uart.read(5) # read up to 5 bytes

Two UARTs are available. UART0 is on Pins 1 (TX) and 3 (RX). UART0 is
bidirectional, and by default is used for the REPL. UART1 is on Pins 2
(TX) and 8 (RX) however Pin 8 is used to connect the flash chip, so
UART1 is TX only.

When UART0 is attached to the REPL, all incoming chars on UART(0) go
straight to stdin so uart.read() will always return None.  Use
sys.stdin.read() if it's needed to read characters from the UART(0)
while it's also used for the REPL (or detach, read, then reattach).
When detached the UART(0) can be used for other purposes.

If there are no objects in any of the dupterm slots when the REPL is started (on
:doc:`hard or soft reset </reference/reset_boot>`) then UART(0) is automatically
attached. Without this, the only way to recover a board without a REPL would be
to completely erase and reflash (which would install the default boot.py which
attaches the REPL).

To detach the REPL from UART0, use::

    import os
    os.dupterm(None, 1)

The REPL is attached by default. If you have detached it, to reattach
it use::

    import os, machine
    uart = machine.UART(0, 115200)
    os.dupterm(uart, 1)

----------------------------


Use the ``machine.PWM`` class::

    from machine import Pin, PWM



----------------------------------


Use the :ref:`machine.ADC <machine.ADC>` class::

    from machine import ADC


----------------


    from machine import Pin, SoftSPI








----------------


    from machine import Pin, SPI

    hspi = SPI(1, baudrate=80000000, polarity=0, phase=0)


-------

The I2C driver is implemented in software and works on all pins,
and is accessed via the :ref:`machine.I2C <machine.I2C>` class (which is an
alias of :ref:`machine.SoftI2C <machine.SoftI2C>`)::

    from machine import Pin, I2C

    # construct an I2C bus
    i2c = I2C(scl=Pin(5), sda=Pin(4), freq=100000)



---------------------

See :ref:`machine.RTC <machine.RTC>` ::

    from machine import RTC

    rtc = RTC()
    rtc.datetime((2017, 8, 23, 0, 1, 12, 48, 0)) # set a specific date and
                                                 # time, eg. 2017/8/23 1:12:48
                                                 # the day-of-week value is ignored
    rtc.datetime() # get date and time

    # synchronize with ntp
    # need to be connected to wifi
    import ntptime
    ntptime.settime() # set the rtc datetime from the remote server
    rtc.datetime()    # get the date and time in UTC

.. note:: Not all methods are implemented: `RTC.now()`, `RTC.irq(handler=*) <RTC.irq>`
          (using a custom handler), `RTC.init()` and `RTC.deinit()` are
          currently not supported.

WDT (Watchdog timer)
--------------------

See :ref:`machine.WDT <machine.WDT>`. ::

    from machine import WDT

    # enable the WDT
    wdt = WDT()
    wdt.feed()

---------------


    import machine

    rtc = machine.RTC()
    rtc.irq(trigger=rtc.ALARM0, wake=machine.DEEPSLEEP)

    if machine.reset_cause() == machine.DEEPSLEEP_RESET:
        print('woke from a deep sleep')

    rtc.alarm(rtc.ALARM0, 10000)

    machine.deepsleep()



    from machine import Pin
    import onewire



    import time, ds18x20
    ds = ds18x20.DS18X20(ow)
    roms = ds.scan()
    ds.convert_temp()
    time.sleep_ms(750)
    for rom in roms:
        print(ds.read_temp(rom))



Use the ``neopixel`` module::

    from machine import Pin
    from neopixel import NeoPixel


.. Warning::
   By default ``NeoPixel`` is configured to control the more popular *800kHz*
   units. It is possible to use alternative timing to control other (typically
   400kHz) devices by passing ``timing=0`` when constructing the
   ``NeoPixel`` object.

For low-level driving of a NeoPixel see `machine.bitstream`.

APA102 driver
-------------

Use the ``apa102`` module::

    from machine import Pin
    from apa102 import APA102

    clock = Pin(14, Pin.OUT)     # set GPIO14 to output to drive the clock
    data = Pin(13, Pin.OUT)      # set GPIO13 to output to drive the data
    apa = APA102(clock, data, 8) # create APA102 driver on the clock and the data pin for 8 pixels
    apa[0] = (255, 255, 255, 31) # set the first pixel to white with a maximum brightness of 31
    apa.write()                  # write data to all pixels
    r, g, b, brightness = apa[0] # get first pixel colour

For low-level driving of an APA102::

    import esp
    esp.apa102_write(clock_pin, data_pin, rgbi_buf)

----------


    import dht
    import machine

    d = dht.DHT11(machine.Pin(4))
    d.measure()
    d.temperature() # eg. 23 (°C)
    d.humidity()    # eg. 41 (% RH)

    d = dht.DHT22(machine.Pin(4))
    d.measure()
    d.temperature() # eg. 23.6 (°C)
    d.humidity()    # eg. 41.3 (% RH)

SSD1306 driver
--------------

Driver for SSD1306 monochrome OLED displays. See tutorial :ref:`ssd1306`. ::

    from machine import Pin, I2C
    import ssd1306

    i2c = I2C(scl=Pin(5), sda=Pin(4), freq=100000)
    display = ssd1306.SSD1306_I2C(128, 64, i2c)

    display.text('Hello World', 0, 0, 1)
    display.show()

----------------------------------------


    import webrepl_setup


    import webrepl
    webrepl.start()



