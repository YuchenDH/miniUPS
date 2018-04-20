from django.contrib import admin

# Register your models here.
from .models import trucks,orders,items

admin.site.register(trucks)
admin.site.register(orders)
admin.site.register(items)