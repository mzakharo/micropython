:mod:`touch` ---  电容触摸屏
===============================================

.. module:: touch
   :synopsis: 电容触摸屏

这个模块用于控制触摸屏。

.. image:: http://www.01studio.org/micropython/picture/4_3LCD.png
    :alt: 01Studio 4.3' LCD picture
    :width: 700px

相关资料请点击下面链接:

* `01Studio 4.3' LCD 原理图 <http://bbs.01studio.org/micropython/sch/4_3LCD.pdf>`_ (PDF)

GT1151 电容触摸对象
--------------------

GT1151 类提供GT1151电容触摸屏控制接口，通过构建该对象可以轻松实现驱动GT1151电容触摸屏。

示例::

    import touch

    t = touch.GT1151(portrait=1)  #构建触摸屏对象
    t.read()  #获取触摸状态和坐标

构造函数
------------

.. class:: touch.GT1151(portrait=1)

    构建一个GT1151电容触摸屏对象对象，参数如下：

	- ``portrait`` LCD显示方向：
	
		- ``1`` - 竖屏，480*800，开发板默认方向；
		- ``2`` - 横屏，800*480 ，1基础上顺时针旋转90°；	
		- ``3`` - 竖屏，480*800 ，1基础上顺时针旋转180°；		
		- ``4`` - 横屏，800*480 ，1基础上顺时针旋转270°。	

使用方法
--------------

.. method:: GT1151.read()

   读取触摸屏坐标，返回(states,x,y)

	- ``states`` 当前触摸状态：
	
		- ``0`` - Press 按下；
		- ``1`` - Move 移动；
		- ``2`` - Release 松开。

	- ``x`` 横坐标；
	- ``y`` 纵坐标；

.. method:: GT1151.tick_inc()

   触摸屏响应。
	

	
	