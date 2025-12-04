import unittest
import math
import unittest
import sys
import os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import interface

class TestRobotLogic(unittest.TestCase):

    def setUp(self):
        interface.robot_x = 0
        interface.robot_y = 0
        interface.robot_theta = 0
        interface.ovos_entregues = []
        interface.path_points = []

    def test_movimento_frente(self):
        interface.simular_movimento("FRENTE 100", animate=False)
        self.assertAlmostEqual(interface.robot_x, 100)
        self.assertEqual(interface.robot_y, 0)

    def test_giro_e_movimento(self):
        interface.simular_movimento("ESQUERDA 90", animate=False)
        interface.simular_movimento("FRENTE 100", animate=False)
        self.assertAlmostEqual(interface.robot_x, 0, delta=0.1)
        self.assertAlmostEqual(interface.robot_y, 100, delta=0.1)
        self.assertEqual(interface.robot_theta, 90)

    def test_comando_ovo(self):
        interface.simular_movimento("FRENTE 50", animate=False)
        interface.simular_movimento("ENTREGAR", animate=False)
        
        self.assertEqual(len(interface.ovos_entregues), 1)
        expected_x = (interface.CANVAS_WIDTH/2) + (50 * interface.SCALE)
        self.assertEqual(interface.ovos_entregues[0][0], expected_x)

if __name__ == '__main__':
    unittest.main()