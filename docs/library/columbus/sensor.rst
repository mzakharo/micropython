:mod:`sensor` ---  摄像头
===============================================

.. module:: sensor
   :synopsis: 摄像头

这个模块用于控制摄像头。

.. image:: http://www.01studio.org/micropython/picture/STM32_OV2640.png
    :alt: 01Studio STM32 OV2640 Module picture
    :width: 700px

相关资料请点击下面链接:

* `01Studio STM32 OV2640摄像头模块原理图 <http://bbs.01studio.org/micropython/sch/STM32_OV2640.pdf>`_ (PDF)

OV2640 对象
--------------

OV2640 类提供一个OV2640摄像头控制接口，通过构建该对象可以轻松实现OV2640摄像头的应用。 

示例::

    import sensor

    cam = sensor.OV2640() #构建摄像头对象。
    cam.set_framesize(sensor.VGA) #设置帧大小
    cam.snapshot('/flash/test.jpb')  #拍照并保存
	cam.display()  #液晶屏实时显示摄像头图像

构造函数
------------

.. class:: sensor.OV2640(None) 

    构建一个OV2640摄像头对象。
	
	
使用方法
--------------

.. method:: OV2640.reset()

   复位摄像头。

.. method:: OV2640.set_framesize(framesize)

   摄像拍摄尺寸，默认是 sensor.VGA 640*480

	- ``framesize`` 帧尺寸大小：
	
		- ``sensor.QQQVGA`` - 80*60；
		- ``sensor.QQVGA`` - 160x120；
		- ``sensor.QVGA`` - 320x240；
		- ``sensor.VGA`` -  640x480；
		- ``sensor.WVGA`` - 720x480；
		- ``sensor.XGA`` - 1024x768；
		- ``sensor.UXGA`` -  1600x1200；

.. method:: OV2640.set_vflip(value=0)

   设置摄像头垂直翻转：

	- ``value`` 是否开启：
	
		- ``0`` - 关闭垂直翻转；
		- ``1`` - 开启垂直翻转；

.. method:: OV2640.set_hmirror(value=0)

   设置摄像头水平镜像：

	- ``value`` 是否开启：
	
		- ``0`` - 关闭水平镜像；
		- ``1`` - 开启水平镜像；

.. method:: OV2640.snapshot(filename)

   拍摄照片并保存，支持格式 JPG：

	- ``filename`` 路径+名称，如："/flash/1.jpg"、"/sd/2.jpg"

.. method:: OV2640.display()

   LCD实时显示摄像头采集图像。执行该函数会自动初始化LCD。

.. method:: OV2640.display_stop()

   关闭摄像头采集图像LCD实时显示。

.. method:: OV2640.deinit()

   注销对象。