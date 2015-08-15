from django.db import models
import datetime
import pytz

tz = pytz.timezone('US/Mountain')


class Kegerator(models.Model):
    def __unicode__(self):
        return unicode("%s" % self.name)
    name = models.CharField(max_length=50)
    current_keg = models.ForeignKey('Keg', related_name='current_kegerator', null=True, blank=True)


class Keg(models.Model):
    def __unicode__(self):
        return unicode("%s %s (tapped on %s)" % (self.brewer, self.style, self.tap_date.date().strftime("%b %d")))

    brewer = models.CharField(max_length=50)
    style = models.CharField(max_length=50)
    tap_date = models.DateTimeField(default=datetime.datetime.now(tz))
    kick_date = models.DateTimeField(blank=True, null=True)
    kegerator = models.ForeignKey(Kegerator)


class Beer(models.Model):
    def __unicode__(self):
        return unicode("%s Beer #%d" % (self.keg, self.pour_number))

    pour_number = models.IntegerField(default=0)
    pour_date = models.DateTimeField(default=datetime.datetime.now(tz))
    pour_duration = models.IntegerField('Pour Duration (seconds)', default=0)
    keg = models.ForeignKey(Keg)
