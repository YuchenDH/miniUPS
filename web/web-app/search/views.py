from django.shortcuts import render
from django.http import HttpResponse
from.models import trucks,orders,items
import dateutil.parser
# Create your views here.
def search(request):
	if request.method == 'POST':
		tnum = request.POST['tnum']
		status=['created','truck on the way','waiting for warehouse load','out of delivery','delivered']
		imgpath=['nstatus1.png','nstatus2.png','nstatus3.png','nstatus4.png','nstatus5.png']
		if orders.objects.filter(tracking_num = tnum).exists():
			shipment = orders.objects.filter(tracking_num = tnum).all()[0]
			message = status[shipment.status-1]
			date = shipment.date.strftime("%Y-%m-%d %H:%M:%S")
			return render(request,'search/shipmentdetail.html',context={'status':message,'date':date,'imgpath':imgpath[shipment.status-1]})
		else: 
			return render(request,'search/invalid.html')
	return HttpResponse('<h1>Not POST</h1>')