from django.contrib.auth.forms import UserCreationForm
from .models import realuser

class RegisterForm(UserCreationForm):
    class Meta(UserCreationForm.Meta):
        model = realuser
        fields = ("username","email")