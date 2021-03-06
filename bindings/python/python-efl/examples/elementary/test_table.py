#!/usr/bin/env python
# encoding: utf-8

from efl.evas import EVAS_HINT_EXPAND, EVAS_HINT_FILL
from efl import elementary
from efl.elementary.window import StandardWindow
from efl.elementary.background import Background
from efl.elementary.button import Button
from efl.elementary.table import Table
from efl.elementary.box import Box
from efl.elementary.list import List
from efl.elementary.label import Label
from efl.elementary.frame import Frame

EXPAND_BOTH = EVAS_HINT_EXPAND, EVAS_HINT_EXPAND
EXPAND_HORIZ = EVAS_HINT_EXPAND, 0.0
FILL_BOTH = EVAS_HINT_FILL, EVAS_HINT_FILL

def table_clicked(obj, item=None):
    win = StandardWindow("table", "Table", autodel=True)

    tb = Table(win, size_hint_weight=EXPAND_BOTH)
    win.resize_object_add(tb)
    tb.show()

    bt = Button(win, text="Button 1", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 0, 0, 1, 1)
    bt.show()

    bt = Button(win, text="Button 2", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 0, 1, 1)
    bt.show()

    bt = Button(win, text="Button 3", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 2, 0, 1, 1)
    bt.show()

    bt = Button(win, text="Button 4", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 0, 1, 2, 1)
    bt.show()

    bt = Button(win, text="Button 5", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 2, 1, 1, 3)
    bt.show()

    bt = Button(win, text="Button 6", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 0, 2, 2, 2)
    bt.show()

    win.show()

def table2_clicked(obj, item=None):
    win = StandardWindow("table2", "Table Homogeneous", autodel=True)

    tb = Table(win, homogeneous=True, size_hint_weight=EXPAND_BOTH)
    win.resize_object_add(tb)
    tb.show()

    bt = Button(win, text="A", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 1, 2, 2)
    bt.show()

    bt = Button(win, text="Blah blah blah", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 3, 0, 2, 3)
    bt.show()

    bt = Button(win, text="Hallow", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 0, 3, 10, 1)
    bt.show()

    bt = Button(win, text="B", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 2, 5, 2, 1)
    bt.show()

    bt = Button(win, text="C", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 8, 8, 1, 1)
    bt.show()

    bt = Button(win, text="Wide", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 7, 7, 2)
    bt.show()

    win.show()

def my_tb_ch(obj, data):
    win = data
    tb = win.data["tb"]
    b2 = win.data["b2"]

    tb.unpack(b2)
    tb.pack(b2, 1, 0, 1, 2)

def table3_clicked(obj, item=None):
    win = StandardWindow("table3", "Table 3", autodel=True)

    tb = Table(win, size_hint_weight=EXPAND_BOTH)
    win.resize_object_add(tb)
    win.data["tb"] = tb
    tb.show()

    bt = Button(win, text="Button 1", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 0, 0, 1, 1)
    win.data["b1"] = bt
    bt.callback_clicked_add(my_tb_ch, win)
    bt.show()

    bt = Button(win, text="Button 2", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 0, 1, 1)
    win.data["b2"] = bt
    bt.callback_clicked_add(my_tb_ch, win)
    bt.show()

    bt = Button(win, text="Button 3", size_hint_weight=EXPAND_HORIZ,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 0, 1, 1, 1)
    win.data["b3"] = bt
    bt.callback_clicked_add(my_tb_ch, win)
    bt.show()

    win.show()

def table4_clicked(obj, item=None):
    win = StandardWindow("table4", "Table 4", autodel=True)

    tb = Table(win, size_hint_weight=EXPAND_BOTH)
    win.resize_object_add(tb)
    win.data["tb"] = tb
    tb.show()

    bt = Button(win, text="Button 1", size_hint_weight=(0.25, 0.25),
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 0, 0, 1, 1)
    win.data["b1"] = bt
    bt.callback_clicked_add(my_tb_ch, win)
    bt.show()

    bt = Button(win, text="Button 2", size_hint_weight=(0.75, 0.25),
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 0, 1, 1)
    win.data["b2"] = bt
    bt.callback_clicked_add(my_tb_ch, win)
    bt.show()

    bt = Button(win, text="Button 3", size_hint_weight=(0.25, 0.75),
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 0, 1, 1, 1)
    win.data["b3"] = bt
    bt.callback_clicked_add(my_tb_ch, win)
    bt.show()

    win.show()

def table5_clicked(obj, item=None):
    win = StandardWindow("table5", "Table 5", autodel=True)

    tb = Table(win, homogeneous=True, size_hint_weight=EXPAND_BOTH)
    win.resize_object_add(tb)
    tb.show()

    bt = Button(win, text="A", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 33, 0, 34, 33)
    bt.show()

    bt = Button(win, text="B", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 67, 33, 33, 34)
    bt.show()

    bt = Button(win, text="C", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 33, 67, 34, 33)
    bt.show()

    bt = Button(win, text="D", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 0, 33, 33, 34)
    bt.show()

    bt = Button(win, text="X", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 33, 33, 34, 34)
    bt.show()

    win.show()

def table6_clicked(obj, item=None):
    win = StandardWindow("table6", "Table 6", autodel=True)

    tb = Table(win, homogeneous=True, size_hint_weight=EXPAND_BOTH)
    win.resize_object_add(tb)
    tb.show()

    bt = Button(win, text="C", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 1, 2, 2)
    bt.show()

    bt = Button(win, text="A", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 1, 2, 2)
    bt.show()

    bt = Button(win, text="Blah blah blah", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 3, 0, 2, 3)
    bt.show()

    bt = Button(win, text="Hallow", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 0, 3, 10, 1)
    bt.show()

    bt = Button(win, text="B", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 1, 1, 1)
    bt.show()

    bt = Button(win, text="Wide", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 7, 7, 2)
    bt.show()

    win.show()

def table7_clicked(obj, item=None):
    win = StandardWindow("table7", "Table 7", autodel=True)

    tb = Table(win, padding=(10, 20), size_hint_weight=EXPAND_BOTH)
    win.resize_object_add(tb)
    tb.show()

    bt = Button(win, text="C", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 1, 2, 2)
    bt.show()

    bt = Button(win, text="A", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 1, 2, 2)
    bt.show()

    bt = Button(win, text="Blah blah blah", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 3, 0, 2, 3)
    bt.show()

    bt = Button(win, text="Hallow", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 0, 3, 10, 1)
    bt.show()

    bt = Button(win, text="B", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 1, 1, 1)
    bt.show()

    bt = Button(win, text="Wide", size_hint_weight=EXPAND_BOTH,
        size_hint_align=FILL_BOTH)
    tb.pack(bt, 1, 7, 7, 2)
    bt.show()

    win.show()


if __name__ == "__main__":
    elementary.init()
    win = StandardWindow("test", "python-elementary test application",
        size=(320,520))
    win.callback_delete_request_add(lambda o: elementary.exit())

    box0 = Box(win, size_hint_weight=EXPAND_BOTH)
    win.resize_object_add(box0)
    box0.show()

    lb = Label(win)
    lb.text_set("Please select a test from the list below<br>"
                 "by clicking the test button to show the<br>"
                 "test window.")
    lb.show()

    fr = Frame(win, text="Information", content=lb)
    box0.pack_end(fr)
    fr.show()

    items = [
        ("Table", table_clicked),
        ("Table Homogeneous", table2_clicked),
        ("Table 3", table3_clicked),
        ("Table 4", table4_clicked),
        ("Table 5", table5_clicked),
        ("Table 6", table6_clicked),
        ("Table 7", table7_clicked),
        ]

    li = List(win, size_hint_weight=EXPAND_BOTH, size_hint_align=FILL_BOTH)
    box0.pack_end(li)
    li.show()

    for item in items:
        li.item_append(item[0], callback=item[1])

    li.go()

    win.show()
    elementary.run()
    elementary.shutdown()
