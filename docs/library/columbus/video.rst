:mod:`video` ---  视频
===============================================

.. module:: video
   :synopsis: 视频

这个模块用于视频控制。

VIDEO 对象
--------------

VIDEO 类提供avi视频控制接口，通过构建该对象可以轻松实现avi视频的应用。 

示例::

    import video

    v = video.VEDIO()  #构建视频对象
    v.load('/sd/video/badapple.avi')  #加载视频文件
    v.play() #播放视频
    v.pause()  #暂停播放
    v.stop()  #停止播放
    v.volume(80)  #音量设置为80

构造函数
------------

.. class:: video.VIDEO() 

    构建一个VIDEO视频模块对象。
	
	
使用方法
--------------

.. method:: VIDEO.load(filename)

   加载音频文件。支持格式：avi
   
	- ``filename`` 路径+名称，如："VIDEO.load("/sd/test.avi")"

.. method:: VIDEO.play()

   播放视频。执行该函数会进入阻塞。

.. method:: VIDEO.pause()

   暂停播放。

.. method:: VIDEO.continue_play()

   继续播放。暂停后执行该函数可以继续播放。

.. method:: VIDEO.stop()

   停止播放。

.. method:: VIDEO.volume(vol=80)

   调整音量大小，默认80。

	- ``vol`` 音量大小，范围0-100，整数。