#!/usr/bin/env python

from efl import evas
import unittest


class MyObject(evas.SmartObject):
    pass

class CanvasFactory(unittest.TestCase):
    def setUp(self):
        self.canvas = evas.Canvas(method="buffer",
                                  size=(400, 500),
                                  viewport=(0, 0, 400, 500))
        self.canvas.engine_info_set(self.canvas.engine_info_get())
        self.so = MyObject(self.canvas)

    def tearDown(self):
        del self.so
        del self.canvas

    def testRectangle(self):
        obj = self.so.Rectangle(geometry=(10, 20, 30, 40), color="#ff0000")
        self.assertEqual(isinstance(obj, evas.Rectangle), True)
        self.assertEqual(obj.evas, self.canvas)
        self.assertEqual(obj.geometry, (10, 20, 30, 40))
        self.assertEqual(obj.color, (255, 0, 0, 255))

    def testLine(self):
        start = (0, 0)
        end = (100, 200)
        s = (20, 30)
        p = (11, 22)
        obj = self.so.Line(start=start, end=end, size=s, pos=p)
        self.assertEqual(isinstance(obj, evas.Line), True)
        self.assertEqual(obj.evas, self.canvas)
        self.assertEqual(obj.size_get(), s)
        self.assertEqual(obj.pos_get(), p)
        self.assertEqual(obj.xy_get(),
                         (p[0] + start[0],
                          p[1] + start[1],
                          p[0] + end[0],
                          p[1] + end[1]))


if __name__ == '__main__':
    unittest.main(verbosity=2)
    evas.shutdown()
