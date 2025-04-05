:mod:`audio` ---  音频
===============================================

.. module:: audio
   :synopsis: 音频

这个模块用于音频控制。

WM8978 对象
--------------

WM8978 类提供一个WM8978音频芯片控制接口，通过构建该对象可以轻松实现WM8978音频应用。 

示例::

    import audio

    wm = audio.WM8978()  #构建音频对象
    wm.load('/flash/music/Love.mp3')  #加载音频文件
    wm.play() #播放音乐
    wm.pause()  #暂停播放
    wm.stop()  #停止播放
    wm.volume(80)  #音量设置为80
    wm.record('/flash/test.wav', 80) #开始录音，输入增益为80

构造函数
------------

.. class:: audio.WM8978() 

    构建一个WM8978音频模块对象。
	
	
使用方法
--------------

.. method:: WM8978.load(filename)

   加载音频文件。支持格式：mp3/wav
   
	- ``filename`` 路径+名称，如："WM8978.load("/flash/test.mp3")"

.. method:: WM8978.play()

   播放音乐。执行该函数会进入阻塞。

.. method:: WM8978.pause()

   暂停播放。

.. method:: WM8978.continue_play()

   继续播放。暂停后执行该函数可以继续播放。

.. method:: WM8978.stop()

   停止播放。

.. method:: WM8978.volume(vol=80)

   调整音量大小，默认80。

	- ``vol`` 音量大小，范围0-100，整数。
	
.. method:: WM8978.record(filename, db=80)

   录音，麦克风增益默认80。格式：wav

	- ``filename`` 路径+名称，如："WM8978.record("/flash/test.wav")"
	- ``db`` 麦克风输入增益，范围0-100，整数。

.. method:: WM8978.record_stop()

   停止录音