.. _pyboard_general:

=====================================

.. contents::

----------------------------



If needed, you can prevent the use of the SD card by creating an empty file
called ``/flash/SKIPSD``.  If this file exists when the pyboard boots
up then the SD card will be skipped and the pyboard will always boot from the
internal filesystem (in this case the SD card won't be mounted but you can still
mount and use it later in your program using ``vfs.mount``).





----------






---------------------



----------------------------------------

`PDF guide <http://micropython.org/resources/Micro-Python-Windows-setup.pdf>`__.

.. _hardware_index:

.. include:: hardware/index.rst
