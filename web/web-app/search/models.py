from django.db import models
from weblog.models import realuser
# Create your models here.
class trucks(models.Model):
	truck_id = models.IntegerField(primary_key=True)
	status = models.BooleanField(default=True)#True: free  False:busy
	xloction = models.IntegerField(null=True,blank=True)
	yloction = models.IntegerField(null=True,blank=True)

class orders(models.Model):
	tracking_num = models.IntegerField(unique=True)
	order_id = models.IntegerField(primary_key=True)
	wh_id = models.IntegerField(null=True)
	des_x = models.IntegerField(null=True)
	des_y = models.IntegerField(null=True)
	status = models.SmallIntegerField(default=0)#1:created 2:en route 3: waiting at warehouse 4: out for delevery 5: delivered
	date = models.DateTimeField(auto_now=True,editable=True)
	truck = models.ForeignKey(trucks,on_delete=models.CASCADE)
	user = models.ForeignKey(realuser,on_delete=models.CASCADE,null=True)	
	first_item = models.CharField(max_length=50,null=True)
	item_num = models.IntegerField(null=True)

class items(models.Model):
	name = models.CharField(max_length=50)
	order = models.ForeignKey(orders,on_delete=models.CASCADE)