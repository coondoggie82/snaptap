"""
tcp_server.py

Created by William Coon
"""

import sys
sys.path.append('../')

import datetime
import flickrapi
import django
import pytz
from keg_booth import models

django.setup()

tz = pytz.timezone('US/Mountain')

key = open('./flickr/flickr_key.txt').readline().strip()
secret = open('./flickr/flickr_secret.txt').readline().strip()

from twisted.internet import reactor
from twisted.internet import protocol

class Modes:
    WAIT, RXDATA = range(2)

class TCPProtocol(protocol.Protocol):
    # def __init__(self):
    #     self.factory = factory
    mode = Modes.WAIT

    flickr = flickrapi.FlickrAPI(api_key=key, secret=secret)

    def connectionMade(self):
        self.factory.clients.append(self)
        print "%s: New Connection: %s " % (datetime.datetime.utcnow(), self.transport.getPeer())
        print 'Number of connections: %d' % len(self.factory.clients)

    def connectionLost(self, reason=protocol.connectionDone):
        print 'Connection Lost'
        try:
            print '%s Closing connection: %s' % (datetime.datetime.utcnow(), self.transport.getPeer())
            self.factory.clients.remove(self)
        except ValueError:
            print 'Client not found'
            pass

    def dataReceived(self, data):
        if self.mode == Modes.WAIT:
            if 'ECHO:' in data:
                msg = data.split(':')[1]
                print 'Received ECHO: %s' % msg
                self.transport.write(msg)
                return
            elif 'NEWBEER' in data:
                print "Received New Beer"
                self.current_keg = models.Kegerator.objects.get(name__contains="The Studio").current_keg
                number = self.current_keg.beer_set.count()+1
                self.current_beer = models.Beer(pour_number=number, keg=self.current_keg)
                self.current_beer.save()
                self.writefile = open("images/%s.jpg" % self.current_beer, "wb")
                self.transport.write('ACK')
            elif 'SNDDATA' in data:
                print "Starting RX Data mode"
                self.mode = Modes.RXDATA
                self.transport.write('ACK')
            elif 'DURATION' in data:
                duration = int(data.split(':')[1])
                self.current_beer.pour_duration = duration
                self.transport.write('ACK')
            elif 'GETCOUNT' in data:
                now = datetime.datetime.now(tz)
                if 'DAY' in data:
                    today8am = now.replace(hour=8, minute=0, second=0, microsecond=0)
                    yesterday8am = today8am - datetime.timedelta(1)
                    if now < today8am:
                        start = yesterday8am
                    else:
                        start = today8am
                    day_count = models.Beer.objects.filter(pour_date__gte=start).count()
                    print "Sending Day Count %d" % day_count
                    self.transport.write("%d" % day_count)
                elif 'WEEK' in data:
                    monday8am = (now - datetime.timedelta(days=now.weekday())).replace(hour=8,
                                                                                       minute=0,
                                                                                       second=0,
                                                                                       microsecond=0)
                    week_count = models.Beer.objects.filter(pour_date__gte=monday8am).count()
                    print "Sending Week Count %d" % week_count
                    self.transport.write("%d" % week_count)
                elif 'MONTH' in data:
                    firstOfMonth8am = (now - datetime.timedelta(days=(now.day-1))).replace(hour=8,
                                                                                           minute=0,
                                                                                           second=0,
                                                                                           microsecond=0)
                    month_count = models.Beer.objects.filter(pour_date__gte=firstOfMonth8am).count()
                    print "Sending Month Count %d" % month_count
                    self.transport.write("%d" % month_count)
                elif 'KEG' in data:
                    self.current_keg = models.Kegerator.objects.get(name__contains="The Studio").current_keg
                    keg_count = self.current_keg.beer_set.count()
                    print "Sending Keg Count %d" % keg_count
                    self.transport.write("%d" % keg_count)

        elif self.mode == Modes.RXDATA:
            if 'ENDDATA' in data:
                print "End of data, closing file"
                self.writefile.close()
                self.writefile = ''
                self.flickr.upload(filename="images/%s.jpg" % self.current_beer, public=1, title=self.current_beer)
                self.mode = Modes.WAIT
                self.transport.write('ACK')
            elif self.writefile:
                print "Writing data"
                self.writefile.write(data)


def run():
    factory = protocol.Factory()
    factory.protocol = TCPProtocol
    factory.clients = []
    factory.games = {}
    reactor.listenTCP(443, factory)
    print 'Phone TCP Server Started'
    reactor.run()


if __name__ == "__main__":
    run()
