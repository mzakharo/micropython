.. _esp32_quickref:

=============================




.. toctree::
   :maxdepth: 1

   general.rst
   tutorial/index.rst

Note that there are several varieties of ESP32 -- ESP32, ESP32C3, ESP32C6, ESP32S2, ESP32S3 --
supported by MicroPython, with some differences in functionality between them.

Installing MicroPython
----------------------


---------------------


The :mod:`machine` module::

    import machine


The :mod:`esp` module::

    import esp


    esp.flash_size()
    esp.flash_user_start()
    esp.flash_erase(sector_no)
    esp.flash_write(byte_offset, buffer)
    esp.flash_read(byte_offset, buffer)

The :mod:`esp32` module::

    import esp32



ESP32C3, ESP32C6, ESP32S2, and ESP32S3 also have an internal temperature sensor available.
It is implemented a bit differently to the ESP32 and returns the temperature in
Celsius::

    esp32.mcu_temperature() # read the internal temperature of the MCU, in Celsius

Networking
----------

WLAN
^^^^

The :class:`network.WLAN` class in the :mod:`network` module::

    import network

    wlan = network.WLAN()       # create station interface (the default, see below for an access point interface)
    wlan.active(True)           # activate the interface
    wlan.scan()                 # scan for access points
    wlan.isconnected()          # check if the station is connected to an AP
    wlan.connect('ssid', 'key') # connect to an AP
    wlan.config('mac')          # get the interface's MAC address
    wlan.ipconfig('addr4')      # get the interface's IPv4 addresses

    ap = network.WLAN(network.WLAN.IF_AP) # create access-point interface
    ap.config(ssid='ESP-AP')              # set the SSID of the access point
    ap.config(max_clients=10)             # set how many clients can connect to the network
    ap.active(True)                       # activate the interface


    def do_connect():
        import machine, network
        wlan = network.WLAN()
        wlan.active(True)
        if not wlan.isconnected():
            print('connecting to network...')
            wlan.connect('ssid', 'key')
            while not wlan.isconnected():
                machine.idle()
        print('network config:', wlan.ipconfig('addr4'))

Once the network is established the :mod:`socket <socket>` module can be used
to create and use TCP/UDP sockets as usual, and the ``requests`` module for
convenient HTTP requests.

After a call to ``wlan.connect()``, the device will by default retry to connect
**forever**, even when the authentication failed or no AP is in range.
``wlan.status()`` will return ``network.STAT_CONNECTING`` in this state until a
connection succeeds or the interface gets disabled.  This can be changed by
calling ``wlan.config(reconnects=n)``, where n are the number of desired reconnect
attempts (0 means it won't retry, -1 will restore the default behaviour of trying
to reconnect forever).

.. _esp32_network_lan:

LAN
^^^

Built-in MAC (original ESP32)
"""""""""""""""""""""""""""""

The original ESP32 SoC has a built-in Ethernet MAC. Using this MAC requires an
external Ethernet PHY to be wired to the chip's EMAC pins. Most of the EMAC pin
assignments are fixed, consult the ESP32 datasheet for details.

If the PHY is connected, the internal Ethernet MAC can be configured via
the :class:`network.LAN` constructor::

    import network

    lan = network.LAN(mdc=PIN_MDC, ...)   # Set the pin and mode configuration
    lan.active(True)                      # activate the interface
    lan.ipconfig('addr4')                 # get the interface's IPv4 addresses


Required keyword arguments for the constructor:

- ``mdc`` and ``mdio`` - :class:`machine.Pin` objects (or integers) specifying
  the MDC and MDIO pins.
- ``phy_type`` - Select the PHY device type. Supported devices are
  ``PHY_LAN8710``, ``PHY_LAN8720``, ``PHY_IP101``, ``PHY_RTL8201``,
  ``PHY_DP83848``, ``PHY_KSZ8041`` and ``PHY_KSZ8081``. These values are all
  constants defined in the ``network`` module.
- ``phy_addr`` - The address number of the PHY device. Must be an integer in the
  range 0x00 to 0x1f, inclusive. Common values are ``0`` and ``1``.

All of the above keyword arguments must be present to configure the interface.

Optional keyword arguments:

- ``reset`` - :class:`machine.Pin` object (or integer) specifying the PHY reset pin.
- ``power`` - :class:`machine.Pin` object (or integer) specifying a pin which
  switches the power of the PHY device.
- ``ref_clk`` - :class:`machine.Pin` object (or integer) specifying the pin used
  for the EMAC ``ref_clk`` signal. If not specified, the board default is used
  (typically GPIO 0, but may be different if a particular board has Ethernet.)
- ``ref_clk_mode`` - Defines whether the EMAC ``ref_clk`` pin of the ESP32
  should be an input or an output. Suitable values are ``machine.Pin.IN`` and
  ``machine.Pin.OUT``. If not specified, the board default is used
  (typically input, but may be different if a particular board has Ethernet.)

These are working configurations for LAN interfaces of some popular ESP32 boards::

    # Olimex ESP32-GATEWAY: power controlled by Pin(5)
    # Olimex ESP32 PoE and ESP32-PoE ISO: power controlled by Pin(12)

    lan = network.LAN(mdc=machine.Pin(23), mdio=machine.Pin(18), power=machine.Pin(5),
                      phy_type=network.PHY_LAN8720, phy_addr=0,
                      ref_clk=machine.Pin(17), ref_clk_mode=machine.Pin.OUT)

    # Wireless-Tag's WT32-ETH01

    lan = network.LAN(mdc=machine.Pin(23), mdio=machine.Pin(18),
                      phy_type=network.PHY_LAN8720, phy_addr=1, power=None)

    # Wireless-Tag's WT32-ETH01 v1.4

    lan = network.LAN(mdc=machine.Pin(23), mdio=machine.Pin(18),
                      phy_type=network.PHY_LAN8720, phy_addr=1,
                      power=machine.Pin(16))

    # Espressif ESP32-Ethernet-Kit_A_V1.2

    lan = network.LAN(id=0, mdc=Pin(23), mdio=Pin(18), power=Pin(5),
                      phy_type=network.PHY_IP101, phy_addr=1)


.. _esp32_spi_ethernet:

SPI Ethernet Interface
""""""""""""""""""""""

All ESP32 SoCs support external SPI Ethernet interface chips. These are Ethernet
interfaces that connect via a SPI bus, rather than an Ethernet RMII interface.

.. note:: The only exception is the ESP32 ``d2wd`` variant, where this feature is disabled
     to save code size.

SPI Ethernet uses the same :class:`network.LAN` constructor, with a different
set of keyword arguments::

    import machine, network

    spi = machine.SPI(1, sck=SCK_PIN, mosi=MOSI_PIN, miso=MISO_PIN)
    lan = network.LAN(spi=spi, cs=CS_PIN, ...)   # Set the pin and mode configuration
    lan.active(True)                             # activate the interface
    lan.ipconfig('addr4')                        # get the interface's IPv4 addresses

Required keyword arguments for the constructor:

- ``spi`` - Should be a :class:`machine.SPI` object configured for this
  connection. Note that any clock speed configured on the SPI object is ignored,
  the SPI Ethernet clock speed is configured at compile time.
- ``cs`` - :class:`machine.Pin` object (or integer) specifying the CS pin
  connected to the interface.
- ``int`` - :class:`machine.Pin` object (or integer) specifying the INT pin
  connected to the interface.
- ``phy_type`` - Select the SPI Ethernet interface type. Supported devices are
  ``PHY_KSZ8851SNL``, ``PHY_DM9051``, ``PHY_W5500``. These values are all
  constants defined in the ``network`` module.
- ``phy_addr`` - The address number of the PHY device. Must be an integer in the
  range 0x00 to 0x1f, inclusive. This is usually ``0`` for SPI Ethernet devices.

All of the above keyword arguments must be present to configure the interface.

Optional keyword arguments for the constructor:

- ``reset`` - :class:`machine.Pin` object (or integer) specifying the SPI Ethernet
  interface reset pin.
- ``power`` - :class:`machine.Pin` object (or integer) specifying a pin which
  switches the power of the SPI Ethernet interface.

Here is a sample configuration for a WIZNet W5500 chip connected to pins on
an ESP32-S3 development board::

    import machine, network
    from machine import Pin, SPI

    spi = SPI(1, sck=Pin(12), mosi=Pin(13), miso=Pin(14))
    lan = network.LAN(spi=spi, phy_type=network.PHY_W5500, phy_addr=0,
                      cs=Pin(10), int=Pin(11))

.. note:: WIZnet W5500 Ethernet is also supported on some other MicroPython
          ports, but using a :ref:`different software interface
          <network.WIZNET5K>`.

Delay and timing
----------------

Use the :mod:`time <time>` module::

    import time


定时器
------

    from machine import Timer

    tim0 = Timer(0)
    tim0.init(period=5000, mode=Timer.ONE_SHOT, callback=lambda t:print(0))

    tim1 = Timer(1)
    tim1.init(period=2000, mode=Timer.PERIODIC, callback=lambda t:print(1))


Virtual timers are not currently supported on this port.

.. _Pins_and_GPIO:

-------------


    from machine import Pin





Four drive strengths are supported, using the ``drive`` keyword argument to the
``Pin()`` constructor or ``Pin.init()`` method, with different corresponding
safe maximum source/sink currents and approximate internal driver resistances:

 - ``Pin.DRIVE_0``: 5mA / 130 ohm
 - ``Pin.DRIVE_1``: 10mA / 60 ohm
 - ``Pin.DRIVE_2``: 20mA / 30 ohm (default strength if not configured)
 - ``Pin.DRIVE_3``: 40mA / 15 ohm

The ``hold=`` keyword argument to ``Pin()`` and ``Pin.init()`` will enable the
ESP32 "pad hold" feature. When set to ``True``, the pin configuration
(direction, pull resistors and output value) will be held and any further
changes (including changing the output level) will not be applied. Setting
``hold=False`` will immediately apply any outstanding pin configuration changes
and release the pin. Using ``hold=True`` while a pin is already held will apply
any configuration changes and then immediately reapply the hold.

Notes:




* See :ref:`Deep_sleep_Mode` for a discussion of pin behaviour during sleep

There's a higher-level abstraction :ref:`machine.Signal <machine.Signal>`
which can be used to invert a pin. Useful for illuminating active-low LEDs
using ``on()`` or ``value(1)``.

UART (serial bus)
-----------------

See :ref:`machine.UART <machine.UART>`. ::

    from machine import UART

    uart1 = UART(1, baudrate=9600, tx=33, rx=32)
    uart1.write('hello')  # write 5 bytes
    uart1.read(5)         # read up to 5 bytes

The ESP32 has three hardware UARTs: UART0, UART1 and UART2.
They each have default GPIO assigned to them, however depending on your
ESP32 variant and board, these pins may conflict with embedded flash,
onboard PSRAM or peripherals.

Any GPIO can be used for hardware UARTs using the GPIO matrix, except for
input-only pins 34-39 that can be used as ``rx``. To avoid conflicts simply
provide ``tx`` and ``rx`` pins when constructing. The default pins listed
below.

=====  =====  =====  =====
\      UART0  UART1  UART2
=====  =====  =====  =====
tx     1      10     17
rx     3      9      16
=====  =====  =====  =====

----------------------------


Use the :ref:`machine.PWM <machine.PWM>` class::

    from machine import Pin, PWM

    pwm0 = PWM(Pin(0), freq=5000, duty_u16=32768) # create PWM object from a pin
    freq = pwm0.freq()         # get current frequency
    pwm0.freq(1000)            # set PWM frequency from 1Hz to 40MHz

    duty = pwm0.duty()         # get current duty cycle, range 0-1023 (default 512, 50%)
    pwm0.duty(256)             # set duty cycle from 0 to 1023 as a ratio duty/1023, (now 25%)

    duty_u16 = pwm0.duty_u16() # get current duty cycle, range 0-65535
    pwm0.duty_u16(2**16*3//4)  # set duty cycle from 0 to 65535 as a ratio duty_u16/65535, (now 75%)

    duty_ns = pwm0.duty_ns()   # get current pulse width in ns
    pwm0.duty_ns(250_000)      # set pulse width in nanoseconds from 0 to 1_000_000_000/freq, (now 25%)

    pwm0.deinit()              # turn off PWM on the pin

    pwm2 = PWM(Pin(2), freq=20000, duty=512)  # create and configure in one go
    print(pwm2)                               # view PWM settings

ESP chips have different hardware peripherals:

=====================================================  ========  ========  ========
Hardware specification                                    ESP32  ESP32-S2  ESP32-C3
-----------------------------------------------------  --------  --------  --------
Number of groups (speed modes)                                2         1         1
Number of timers per group                                    4         4         4
Number of channels per group                                  8         8         6
-----------------------------------------------------  --------  --------  --------
Different PWM frequencies (groups * timers)                   8         4         4
Total PWM channels (Pins, duties) (groups * channels)        16         8         6
=====================================================  ========  ========  ========

A maximum number of PWM channels (Pins) are available on the ESP32 - 16 channels,
but only 8 different PWM frequencies are available, the remaining 8 channels must
have the same frequency.  On the other hand, 16 independent PWM duty cycles are
possible at the same frequency.

See more examples in the :ref:`esp32_pwm` tutorial.

DAC (digital to analog conversion)
----------------------------------

On the ESP32, DAC functionality is available on pins 25, 26.
On the ESP32S2, DAC functionality is available on pins 17, 18.

Use the DAC::

    from machine import DAC, Pin

    dac = DAC(Pin(25))  # create an DAC object acting on a pin
    dac.write(128)      # set a raw analog value in the range 0-255, 50% now

ADC (analog to digital conversion)
----------------------------------

On the ESP32, ADC functionality is available on pins 32-39 (ADC block 1) and
pins 0, 2, 4, 12-15 and 25-27 (ADC block 2).

Use the :ref:`machine.ADC <machine.ADC>` class::

    from machine import ADC

    adc = ADC(pin)        # create an ADC object acting on a pin
    val = adc.read_u16()  # read a raw analog value in the range 0-65535
    val = adc.read_uv()   # read an analog value in microvolts

ADC block 2 is also used by WiFi and so attempting to read analog values from
block 2 pins when WiFi is active will raise an exception.

The internal ADC reference voltage is typically 1.1V, but varies slightly from
package to package. The ADC is less linear close to the reference voltage
(particularly at higher attenuations) and has a minimum measurement voltage
around 100mV, voltages at or below this will read as 0. To read voltages
accurately, it is recommended to use the ``read_uv()`` method (see below).

ESP32-specific ADC class method reference:

.. class:: ADC(pin, *, atten)

    Return the ADC object for the specified pin. ESP32 does not support
    different timings for ADC sampling and so the ``sample_ns`` keyword argument
    is not supported.

    To read voltages above the reference voltage, apply input attenuation with
    the ``atten`` keyword argument. Valid values (and approximate linear
    measurement ranges) are:

      - ``ADC.ATTN_0DB``: No attenuation (100mV - 950mV)
      - ``ADC.ATTN_2_5DB``: 2.5dB attenuation (100mV - 1250mV)
      - ``ADC.ATTN_6DB``: 6dB attenuation (150mV - 1750mV)
      - ``ADC.ATTN_11DB``: 11dB attenuation (150mV - 2450mV)

.. Warning::

For compatibility, the ``ADC`` object also provides constants matching the
supported ADC resolutions:

  - ``ADC.WIDTH_9BIT`` = 9
  - ``ADC.WIDTH_10BIT`` = 10
  - ``ADC.WIDTH_11BIT`` = 11
  - ``ADC.WIDTH_12BIT`` = 12


----------------


    from machine import Pin, SoftSPI

    spi = SoftSPI(baudrate=100000, polarity=1, phase=0, sck=Pin(0), mosi=Pin(2), miso=Pin(4))






.. Warning::

----------------


=====  ===========  ============
\      HSPI (id=1)   VSPI (id=2)
=====  ===========  ============
sck    14           18
mosi   13           23
miso   12           19
=====  ===========  ============

Hardware SPI is accessed via the :ref:`machine.SPI <machine.SPI>` class and
has the same methods as software SPI above::

    from machine import Pin, SPI

    hspi = SPI(1, 10000000)
    hspi = SPI(1, 10000000, sck=Pin(14), mosi=Pin(13), miso=Pin(12))
    vspi = SPI(2, baudrate=80000000, polarity=0, phase=0, bits=8, firstbit=0, sck=Pin(18), mosi=Pin(23), miso=Pin(19))

----------------


    from machine import Pin, SoftI2C

    i2c = SoftI2C(scl=Pin(5), sda=Pin(4), freq=100000)




Hardware I2C bus

There are two hardware I2C peripherals with identifiers 0 and 1.  Any available
output-capable pins can be used for SCL and SDA but the defaults are given
below.

=====  ===========  ============
\      I2C(0)       I2C(1)
=====  ===========  ============
scl    18           25
sda    19           26
=====  ===========  ============

The driver is accessed via the :ref:`machine.I2C <machine.I2C>` class and
has the same methods as software I2C above::

    from machine import Pin, I2C

    i2c = I2C(0)
    i2c = I2C(1, scl=Pin(5), sda=Pin(4), freq=400000)

I2S bus
-------

See :ref:`machine.I2S <machine.I2S>`. ::

    from machine import I2S, Pin

    i2s = I2S(0, sck=Pin(13), ws=Pin(14), sd=Pin(34), mode=I2S.TX, bits=16, format=I2S.STEREO, rate=44100, ibuf=40000) # create I2S object
    i2s.write(buf)             # write buffer of audio samples to I2S device

    i2s = I2S(1, sck=Pin(33), ws=Pin(25), sd=Pin(32), mode=I2S.RX, bits=16, format=I2S.MONO, rate=22050, ibuf=40000) # create I2S object
    i2s.readinto(buf)          # fill buffer with audio samples from I2S device

The I2S class is currently available as a Technical Preview.  During the preview period, feedback from
users is encouraged.  Based on this feedback, the I2S class API and implementation may be changed.

ESP32 has two I2S buses with id=0 and id=1


See :ref:`machine.RTC <machine.RTC>` ::

    from machine import RTC

    rtc = RTC()

WDT (Watchdog timer)
--------------------

See :ref:`machine.WDT <machine.WDT>`. ::

    from machine import WDT

    # enable the WDT with a timeout of 5s (1s is the minimum)
    wdt = WDT(timeout=5000)
    wdt.feed()

.. _Deep_sleep_mode:

Deep-sleep mode
---------------


    import machine

    if machine.reset_cause() == machine.DEEPSLEEP_RESET:
        print('woke from a deep sleep')

    machine.deepsleep(10000)


* Calling ``deepsleep()`` without an argument will put the device to sleep
  indefinitely
* A software reset does not change the reset cause

Some ESP32 pins (0, 2, 4, 12-15, 25-27, 32-39) are connected to the RTC during
deep-sleep and can be used to wake the device with the ``wake_on_`` functions in
the :mod:`esp32` module. The output-capable RTC pins (all except 34-39) will
also retain their pull-up or pull-down resistor configuration when entering
deep-sleep.

If the pull resistors are not actively required during deep-sleep and are likely
to cause current leakage (for example a pull-up resistor is connected to ground
through a switch), then they should be disabled to save power before entering
deep-sleep mode::

    from machine import Pin, deepsleep

    # configure input RTC pin with pull-up on boot
    pin = Pin(2, Pin.IN, Pin.PULL_UP)

    # disable pull-up and put the device to sleep for 10 seconds
    pin.init(pull=None)
    machine.deepsleep(10000)

Output-configured RTC pins will also retain their output direction and level in
deep-sleep if pad hold is enabled with the ``hold=True`` argument to
``Pin.init()``.

Non-RTC GPIO pins will be disconnected by default on entering deep-sleep.
Configuration of non-RTC pins - including output level - can be retained by
enabling pad hold on the pin and enabling GPIO pad hold during deep-sleep::

    from machine import Pin, deepsleep
    import esp32

    opin = Pin(19, Pin.OUT, value=1, hold=True) # hold output level
    ipin = Pin(21, Pin.IN, Pin.PULL_UP, hold=True) # hold pull-up

    # enable pad hold in deep-sleep for non-RTC GPIO
    esp32.gpio_deep_sleep_hold(True)

    # put the device to sleep for 10 seconds
    deepsleep(10000)

The pin configuration - including the pad hold - will be retained on wake from
sleep. See :ref:`Pins_and_GPIO` above for a further discussion of pad holding.

SD card
-------

See :ref:`machine.SDCard <machine.SDCard>`. ::

    import machine, os, vfs

    # On original ESP32, slot 2 uses pins sck=18, cs=5, miso=19, mosi=23
    sd = machine.SDCard(slot=2)
    vfs.mount(sd, '/sd') # mount

    os.listdir('/sd')    # list directory contents

    vfs.umount('/sd')    # eject

RMT

The RMT is ESP32-specific and allows generation of accurate digital pulses with
12.5ns resolution.  See :ref:`esp32.RMT <esp32.RMT>` for details.  Usage is::

    import esp32
    from machine import Pin

    r = esp32.RMT(0, pin=Pin(18), clock_div=8)
    r   # RMT(channel=0, pin=18, source_freq=80000000, clock_div=8)
    # The channel resolution is 100ns (1/(source_freq/clock_div)).
    r.write_pulses((1, 20, 2, 40), 0) # Send 0 for 100ns, 1 for 2000ns, 0 for 200ns, 1 for 4000ns



    from machine import Pin
    import onewire



    import time, ds18x20
    ds = ds18x20.DS18X20(ow)
    roms = ds.scan()
    ds.convert_temp()
    time.sleep_ms(750)
    for rom in roms:
        print(ds.read_temp(rom))


NeoPixel and APA106 driver
--------------------------

Use the ``neopixel`` and ``apa106`` modules::

    from machine import Pin
    from neopixel import NeoPixel


The APA106 driver extends NeoPixel, but internally uses a different colour order::

    from apa106 import APA106
    ap = APA106(pin, 8)
    r, g, b = ap[0]

.. Warning::

For low-level driving of a NeoPixel see `machine.bitstream`.
This low-level driver uses an RMT channel by default.  To configure this see
`RMT.bitstream_channel`.

APA102 (DotStar) uses a different driver as it has an additional clock pin.

----------------

ESP32, ESP32-S2 and ESP32-S3 support capacitive touch via the ``TouchPad`` class
in the ``machine`` module::

    from machine import TouchPad, Pin

    t = TouchPad(Pin(14))

``TouchPad.read`` returns a value proportional to the capacitance between the
pin and the board's Ground connection. On ESP32 the number becomes smaller when
the pin (or connected touch pad) is touched, on ESP32-S2 and ESP32-S3 the number
becomes larger when the pin is touched.

In all cases, a touch causes a significant change in the return value. Note the
returned values are *relative* and can vary depending on the board and
surrounding environment so some calibration (i.e. comparison to a baseline or
rolling average) may be required.

========= ==============================================
Chip      Touch-enabled pins
--------- ----------------------------------------------
ESP32     0, 2, 4, 12, 13, 14, 15, 27, 32, 33
ESP32-S2  1 to 14 inclusive
ESP32-S3  1 to 14 inclusive
========= ==============================================

Trying to assign to any other pins will result in a ``ValueError``.


    import machine
    from machine import TouchPad, Pin
    import esp32

    t = TouchPad(Pin(14))
    esp32.wake_on_touch(True)

<https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/touch_pad.html>`_.


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



    import webrepl_setup


    import webrepl
    webrepl.start()

    webrepl.start(password='mypass')



