"""ups URL Configuration

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/2.0/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  path('', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  path('', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.urls import include, path
    2. Add a URL to urlpatterns:  path('blog/', include('blog.urls'))
"""
from django.contrib import admin,staticfiles
from .settings import BASE_DIR
from django.urls import include,path
from weblog import views
from django.views import static
import os,django

urlpatterns = [
    path('admin/', admin.site.urls),
    path('users/', include('weblog.urls')),
    path('users/', include('django.contrib.auth.urls')),#built-in login change-password find-password view function
    path('',views.index,name='index'),
    path('static/image/<path>',static.serve),
]
