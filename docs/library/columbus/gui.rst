:mod:`gui` ---  可视化控件
===============================================

.. module:: gui
   :synopsis: 可视化控件

这个模块用于可视化控件。

TouchButton 对象
------------------

TouchButton 类提供触摸按钮控制接口，通过构建该对象可以轻松显示屏触摸按钮应用。 

示例::

    import gui

    B1 = gui.TouchButton(0,0,120,80,(255,0,0),'LED3',(255,255,255),fun1) #构建一个按钮
    print(B1.ID())  #打印按钮编号

构造函数
------------

.. class:: gui.TouchButton(x, y, width, height, color, label, label_color, callback) 

    构建触摸按钮对象。构建前需要先初始化LCD和触摸屏。

	- ``x`` 按钮起始横坐标；
	- ``y`` 按钮起始纵坐标；
	- ``width`` 按钮宽度；
	- ``height`` 按钮高度；
	- ``color`` 按钮颜色；
	- ``label`` 按钮标签；
	- ``label_color`` 标签字体颜色；
	- ``callback`` 按钮回调函数，按下松开后触发；
	
	
使用方法
--------------

.. method:: TouchButton.ID()

   返回按钮ID编号。