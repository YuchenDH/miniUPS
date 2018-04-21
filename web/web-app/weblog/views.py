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
    return render(request,'weblog/detail.html',context={'uos':uos})