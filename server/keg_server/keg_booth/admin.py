from django.contrib import admin
from keg_booth import models


class KegeratorAdmin(admin.ModelAdmin):
    list_display = ['name', 'current_keg']


class KegAdmin(admin.ModelAdmin):
    list_display = ['__unicode__', 'brewer', 'style', 'tap_date', 'kick_date', 'kegerator']
    list_filter = ['brewer', 'style', 'kegerator', 'tap_date']
    search_fields = ['brewer', 'style', 'kegerator']


class BeerAdmin(admin.ModelAdmin):
    list_display = ['__unicode__', 'pour_date', 'pour_duration']

admin.site.register(models.Kegerator, KegeratorAdmin)
admin.site.register(models.Keg, KegAdmin)
admin.site.register(models.Beer, BeerAdmin)
