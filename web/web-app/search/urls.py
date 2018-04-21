from django.urls import path
from . import views

app_name = 'search'
urlpatterns = [
    path('tracking/',views.search,name='tracking'),
    path('orders/',views.detail,name='order'),
]