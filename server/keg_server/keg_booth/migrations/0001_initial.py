# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import models, migrations
import datetime


class Migration(migrations.Migration):

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='Beer',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('pour_number', models.IntegerField(default=0)),
                ('pour_date', models.DateTimeField(default=datetime.datetime(2015, 8, 14, 18, 21, 17, 354715))),
                ('pour_duration', models.IntegerField(default=0)),
            ],
            options={
            },
            bases=(models.Model,),
        ),
        migrations.CreateModel(
            name='Keg',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('brewer', models.CharField(max_length=50)),
                ('style', models.CharField(max_length=50)),
                ('tap_date', models.DateTimeField(default=datetime.datetime(2015, 8, 14, 18, 21, 17, 354071))),
                ('kick_date', models.DateTimeField(null=True, blank=True)),
            ],
            options={
            },
            bases=(models.Model,),
        ),
        migrations.CreateModel(
            name='Kegerator',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('name', models.CharField(max_length=50)),
                ('current_keg', models.ForeignKey(related_name='current_kegerator', to='keg_booth.Keg')),
            ],
            options={
            },
            bases=(models.Model,),
        ),
        migrations.AddField(
            model_name='keg',
            name='kegerator',
            field=models.ForeignKey(to='keg_booth.Kegerator'),
            preserve_default=True,
        ),
        migrations.AddField(
            model_name='beer',
            name='keg',
            field=models.ForeignKey(to='keg_booth.Keg'),
            preserve_default=True,
        ),
    ]
