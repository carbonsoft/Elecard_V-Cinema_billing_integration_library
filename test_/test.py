# -*- coding: utf-8 -*-
import unittest
import json
import ctypes
from ctypes import *

from twisted.web import server, resource
from twisted.internet import reactor


DLL_FILE = '/home/nikolay/clients/megafit/billing/billing.so'
TEST_FOLDER = '/home/nikolay/clients/megafit/billing/test_/'


class StubConfig(ctypes.Structure):
    _fields_ = [("billing_ip", ctypes.c_char_p),
                ("billing_psw", ctypes.c_char_p),
                ("log_file", ctypes.c_char_p),
                # ("fp_log", ctypes.c_int),
                ]


class BillingAction(ctypes.Structure):
    _fields_ = [("ver", ctypes.c_byte),
                ("xworks_module", ctypes.c_char_p),
                ("proto", ctypes.c_int),
                ("event", ctypes.c_int),
                ("session_id", ctypes.c_char_p),
                ("url", ctypes.c_char_p),
                ("ip", ctypes.c_char_p),
                ("_", ctypes.c_char * 1000),
                ]



class TestLoadingNotFoundConfig(unittest.TestCase):
    def test_init_invalid_folder(self):
        library = ctypes.CDLL(DLL_FILE)
        config_p = library.billing_open_instance("/does/not/exists/", None)
        assert config_p == 0, u'Must return NULL if config not found!'


class TestLoadingConfig(unittest.TestCase):
    def test_init_invalid_folder(self):
        library = ctypes.CDLL(DLL_FILE)
        config_p = library.billing_open_instance(TEST_FOLDER, None)
        assert config_p != 0, u'Must load!'

        config = cast(config_p, ctypes.POINTER(StubConfig)).contents
        assert config.billing_ip == '127.0.0.1'
        assert config.billing_psw == '123'
        assert config.log_file == '/home/nikolay/clients/megafit/billing/test_/xworks-billing-stub.log'

        library.billing_close_instance(config_p)


class TestPassEmptyActions(unittest.TestCase):
    def test_init_invalid_folder(self):
        library = ctypes.CDLL(DLL_FILE)
        config_p = library.billing_open_instance(TEST_FOLDER, None)
        assert config_p != 0, u'Must load!'

        for event in filter(lambda x: x != 0x07, range(0x01, 0x0D)):
            action = BillingAction(0x03, "test_module", 0x01, event, "", "url", "127.0.0.1", "123")
            res = library.billing_verify_action(config_p, pointer(action))
            assert res == 1

        library.billing_close_instance(config_p)


class TestPassCheckAction(unittest.TestCase):
    web_call_count = 0

    def _make_action(self, ip, url):
        return BillingAction(0x03, "test_module", 0x01, 0x07, "session", url, ip, "trash")

    def _init_web_server(self):
        class Simple(resource.Resource):
            isLeaf = True
            def render_GET(self, request):
                TestPassCheckAction.web_call_count += 1
                data = {
                    '127.0.0.1': ['one', 'two', 'three'],
                    '127.0.0.2': ['one', 'two'],
                    '127.0.0.3': ['one', 'two', 'NTV'],
                }
                return json.dumps(data)
        site = server.Site(Simple())
        reactor.listenTCP(8082, site)

    def _callback_test(self, library, config_p):
        action = self._make_action("127.0.0.1", "one")
        res = library.billing_verify_action(config_p, pointer(action))
        assert res == 1
        action = self._make_action("127.0.0.1", "two")
        res = library.billing_verify_action(config_p, pointer(action))
        assert res == 1
        action = self._make_action("127.0.0.1", "three")
        res = library.billing_verify_action(config_p, pointer(action))
        assert res == 1
        action = self._make_action("127.0.0.2", "one")
        res = library.billing_verify_action(config_p, pointer(action))
        assert res == 1
        action = self._make_action("127.0.0.2", "two")
        res = library.billing_verify_action(config_p, pointer(action))
        assert res == 1
        action = self._make_action("127.0.0.3", "one")
        res = library.billing_verify_action(config_p, pointer(action))
        assert res == 1
        action = self._make_action("127.0.0.3", "two")
        res = library.billing_verify_action(config_p, pointer(action))
        assert res == 1
        action = self._make_action("127.0.0.3", "NTV")
        res = library.billing_verify_action(config_p, pointer(action))
        assert res == 1
        action = self._make_action("127.0.0.4", "one")
        res = library.billing_verify_action(config_p, pointer(action))
        assert res == 0
        action = self._make_action("127.0.0.1", "_one")
        res = library.billing_verify_action(config_p, pointer(action))
        assert res == 0
        reactor.stop()

    def test_init_invalid_folder(self):
        self._init_web_server()

        library = ctypes.CDLL(DLL_FILE)
        config_p = library.billing_open_instance(TEST_FOLDER, None)
        assert config_p != 0, u'Must load!'

        reactor.callLater(2, self._callback_test, library, config_p)
        reactor.run()

        library.billing_close_instance(config_p)
        assert TestPassCheckAction.web_call_count == 1, u'Web не вызывался!'


if __name__ == '__main__':
    unittest.main()