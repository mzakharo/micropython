:mod:`tftlcd` ---  LCD显示屏
===============================================

.. module:: tftlcd
   :synopsis: LCD显示屏

这个模块用于控制TFT LCD显示屏。

.. image:: http://www.01studio.org/micropython/picture/4_3LCD.png
    :alt: 01Studio 4.3' LCD picture
    :width: 700px

相关资料请点击下面链接:

* `01Studio 4.3' LCD 原理图 <http://bbs.01studio.org/micropython/sch/4_3LCD.pdf>`_ (PDF)

LCD43M 对象
--------------

LCD43M 类提供一个4.3寸MCU屏控制接口，通过构建该对象可以轻松实现4.3寸MCU LCD
的各种画图控制。 

示例::

    import tftlcd

    d = tftlcd.LCD43M(portrait=1)  #构建LCD对象
    d.fill((255, 255, 255))  #填充白色
    d.drawRect(0, 0, 100, 100, (255,0,0))  #画红色矩形
    d.printStr('Hello 01Studio!', 0, 0, (0,255,0))  #写字符
    d.Picture(0, 0, '/flash/curry.jpg')  #显示图片


构造函数
------------

.. class:: tftlcd.LCD43M(portrait=1)

    构建一个4.3寸MCU LCD对象，参数如下：

	- ``portrait`` LCD显示方向：
	
		- ``1`` - 竖屏，480*800，开发板默认方向；
		- ``2`` - 横屏，800*480 ，1基础上顺时针旋转90°；	
		- ``3`` - 竖屏，480*800 ，1基础上顺时针旋转180°；		
		- ``4`` - 横屏，800*480 ，1基础上顺时针旋转270°。	

使用方法
--------------

.. method:: LCD43M.fill(color)

   全屏填充:

	- ``color`` RGB颜色数据，如:(255,0,0)表示红色。

.. method:: LCD43M.drawPixel(x,y,color)

   画点:

	- ``x`` 横坐标；
	- ``y`` 纵坐标；
	- ``color`` RGB颜色数据，如:(255,0,0)表示红色。

.. method:: LCD43M.drawLine(x0,y0,x1,y1,color)

   画线段:

	- ``x0`` 起始横坐标；
	- ``y0`` 起始纵坐标；
	- ``x1`` 结束横坐标；
	- ``y1`` 结束纵坐标；
	- ``color`` RGB颜色数据，如:(255,0,0)表示红色。

.. method:: LCD43M.drawRect(x,y,width,height,color,border=1,fillcolor=None)

   画矩形:

	- ``x`` 起始横坐标；
	- ``y`` 起始纵坐标；
	- ``width`` 宽度；
	- ``height`` 高度；
	- ``color`` 边框颜色，RGB颜色数据，如:(255,0,0)表示红色。
	- ``border`` 边框宽度，单位为像素，默认=1；
	- ``fillcolor`` 填充颜色，RGB颜色数据，如:(255,0,0)表示红色,默认=None表示不填充。	

.. method:: LCD43M.drawCircle(x,y,radius,color,border=1,fillcolor=None)

   画圆:

	- ``x`` 圆心横坐标；
	- ``y`` 圆心纵坐标；
	- ``radius`` 半径；
	- ``color`` 边框颜色，RGB颜色数据，如:(255,0,0)表示红色。
	- ``border`` 边框宽度，单位为像素，默认=1；
	- ``fillcolor`` 填充颜色，RGB颜色数据，如:(255,0,0)表示红色,默认=None表示不填充。

.. method:: LCD43M.printStr(text,x,y,color,backcolor=None,size=2)

   写字符:

	- ``text`` 字符，string类型；
	- ``x`` 起始横坐标；	
	- ``y`` 起始纵坐标；
	- ``color`` 字体颜色，RGB颜色数据，如:(255,0,0)表示红色。
	- ``backcolor`` 字体背景颜色，RGB颜色数据，如:(255,0,0)表示红色,默认=None。
	- ``size`` 字体尺寸，默认=2表示标准尺寸：
	
		- ``1`` - 小号；
		- ``2`` - 标准；
		- ``3`` - 中号；
		- ``4`` - 大号；

.. method:: LCD43M.Picture(x,y,filename)

   显示图片，支持图片格式：jpg/bmp。最大尺寸480*800

	- ``x`` 起始横坐标；
	- ``y`` 起始纵坐标；
	- ``filename`` 路径+名称，如："/flash/cat.jpb"、"/sd/dog.bmp"