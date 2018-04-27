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

def detail(request):
	oid = request.GET.get('oi')
	order = orders.objects.get(order_id = oid)
	if (order.user_id != 0) and  bool(order.user_id) and (order.user_id != request.user.id):
		return render(request,'index.html')
	its = items.objects.filter(order_id = oid).all()
	status=['created','truck on the way','waiting for warehouse load','out of delivery','delivered']
	imgpath=['nstatus1.png','nstatus2.png','nstatus3.png','nstatus4.png','nstatus5.png']
	message = status[order.status-1]
	date = order.date.strftime("%Y-%m-%d %H:%M:%S")
	if request.method == 'POST':
		x = request.POST.get('x')
		y = request.POST.get('y')
		if (not x.isdigit()) or (not y.isdigit()):
			return render(request,'search/badinput.html')
		if order.status > 3:
			return HttpResponse('<h1>Package on the way, can not change destination</h1>')
		else:
			orders.objects.filter(order_id = oid).update(des_x = x,des_y = y)
			norder = orders.objects.get(order_id = oid)
			return render(request,'search/detail.html',context={'status':message,'date':date,'imgpath':imgpath[order.status-1],'order':norder,'items':its})
	else:
		return render(request,'search/detail.html',context={'status':message,'date':date,'imgpath':imgpath[order.status-1],'order':order,'items':its})	
