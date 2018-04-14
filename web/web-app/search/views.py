from django.shortcuts import render
from django.http import HttpResponse
from.models import trucks,shipments
import dateutil.parser
# Create your views here.
def search(request):
	if request.method == 'POST':
		tnum = request.POST['tnum']
		status=['created','truck on the way','waiting for warehouse load','out of delivery','delivered']
		imgpath=['status1.png','status2.png','status3.png','status4.png','status5.png']
		if shipments.objects.filter(shipment_id = tnum).exists():
			shipment = shipments.objects.filter(shipment_id = tnum).all()[0]
			message = status[shipment.status-1]
			date = shipment.date.now().strftime("%Y-%m-%d %H:%M:%S")
			return render(request,'search/shipmentdetail.html',context={'status':message,'date':date,'imgpath':imgpath[shipment.status-1]})
		else: 
			return render(request,'search/invalid.html')
	return HttpResponse('<h1>Not POST</h1>')