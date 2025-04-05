.. currentmodule:: network
.. _network.Ethernet:

class Ethernet -- 哥伦布STM32开发板内部以太网控制
=====================================================

这个模块用于哥伦布STM32开发板内部以太网控制

使用示例::

    import network
    nic = network.Ethernet()
    print(nic.ifconfig())

    # now use socket as usual
    ...

构造函数
------------

.. class:: Ethernet()

   构建以太网对象。

   例如，你可以这么构建::

     nic = network.Ethernet()

使用方法
---------

.. method:: Ethernet.isconnected()

   以太网物联连接已建立返回 ``True``，否则返回``False``。
   
.. method:: Ethernet.ifconfig([(ip, subnet, gateway, dns)])

   获取/自动/手动分配IP地址。不传递任何参数表示获取当前IP地址信息。该函数返回上面4个数据。
   
	- ``dhcp`` 自动分配IP地址：
	- ``(ip, subnet, gateway, dns)`` 手动分配IP地址。

   示例::

     nic.ifconfig(('192.168.0.4', '255.255.255.0', '192.168.0.1', '8.8.8.8'))

