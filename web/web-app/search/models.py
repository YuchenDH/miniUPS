from django.db import models

# Create your models here.
class trucks(models.Model):
	status = models.BooleanField(default=True)#True: free  False:busy
	xLoction = models.IntegerField(null=True,blank=True)
	yLoction = models.IntegerField(null=True,blank=True)

class shipments(models.Model):
	shipment_id = models.IntegerField(unique=True)
	status = models.SmallIntegerField(default=0)#1:created 2:en route 3: waiting at warehouse 4: out for delevery 5: delivered
	date = models.DateTimeField(auto_now=True,editable=False)
	truck = models.ForeignKey(trucks,on_delete=models.CASCADE)