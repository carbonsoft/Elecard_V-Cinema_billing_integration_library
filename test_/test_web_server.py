#!/usr/bin/env python
# -*- coding: utf-8 -*-
import random
from decimal import Decimal as D
import hashlib, binascii
import json

from twisted.web import server, resource
from twisted.internet import reactor
from twisted.web.client import getPage


class Simple(resource.Resource):
    isLeaf = True
    def render_GET(self, request):
        # request.write('ONEONETWO\n\n\n')
        # request.finish()
        data = {
            '127.0.0.1': ['one', 'two', 'three'],
            '127.0.0.2': ['one', 'two'],
            '127.0.0.3': ['one', 'two', 'NTV'],
        }
        return json.dumps(data)


site = server.Site(Simple())
reactor.listenTCP(8089, site)
reactor.run()