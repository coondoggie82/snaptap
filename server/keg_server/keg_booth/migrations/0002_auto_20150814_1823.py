# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import models, migrations
import datetime


class Migration(migrations.Migration):

    dependencies = [
        ('keg_booth', '0001_initial'),
    ]

    operations = [
        migrations.AlterField(
            model_name='beer',
            name='pour_date',
            field=models.DateTimeField(default=datetime.datetime(2015, 8, 14, 18, 23, 11, 684061)),
            preserve_default=True,
        ),
        migrations.AlterField(
            model_name='keg',
            name='tap_date',
            field=models.DateTimeField(default=datetime.datetime(2015, 8, 14, 18, 23, 11, 683430)),
            preserve_default=True,
        ),
        migrations.AlterField(
            model_name='kegerator',
            name='current_keg',
            field=models.ForeignKey(related_name='current_kegerator', blank=True, to='keg_booth.Keg', null=True),
            preserve_default=True,
        ),
    ]
