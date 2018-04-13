from django.db import models

# Create your models here.
from django.contrib.auth.models import AbstractUser

class realuser(AbstractUser):
	#define what youe need except for (username,password,email,first_name,last_name) 
	nickname = models.CharField(max_length=50,blank =True)
	class Meta(AbstractUser.Meta):
		pass