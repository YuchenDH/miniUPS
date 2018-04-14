from django.contrib import admin

# Register your models here.
from .models import trucks,shipments

admin.site.register(trucks)
admin.site.register(shipments)