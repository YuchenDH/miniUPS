from django.shortcuts import render

# Create your views here.
from django.shortcuts import redirect
from django.http import HttpResponse
from .forms import RegisterForm
from .models import realuser
from search.models import trucks,orders,items
def register(request):
    #only when we get a POST request, we need to get the information user submit
    if request.method == 'POST':
        form = RegisterForm(request.POST)
        if form.is_valid():
            form.save()
            return redirect('/')
    else:
        form = RegisterForm()
    return render(request,'weblog/register.html',context={'form':form})
            
def index(request):
    return render(request,'index.html')

def detail(request):
    uid = request.user.id
    uos = orders.objects.filter(user_id = uid).all()   
    if request.method == 'POST':
        date = request.POST['date']
        up = int(str(int(date)+1)+'0')
        down = int(date+'0')
        nuos=[]
        for i in range(len(uos)):
            if (uos[i].tracking_num < up) and (uos[i].tracking_num > down):
                nuos.append(uos[i])
        return render(request,'weblog/detail.html',context={'uos':nuos}) 
    return render(request,'weblog/detail.html',context={'uos':uos})