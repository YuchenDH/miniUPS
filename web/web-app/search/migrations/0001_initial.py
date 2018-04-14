# Generated by Django 2.0.2 on 2018-04-14 00:43

from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    initial = True

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='shipment',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('shipment_id', models.IntegerField(unique=True)),
                ('status', models.SmallIntegerField(default=0)),
                ('date', models.DateField(auto_now=True)),
            ],
        ),
        migrations.CreateModel(
            name='truck',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('status', models.BooleanField(default=True)),
                ('xLoction', models.IntegerField(blank=True)),
                ('yLoction', models.IntegerField(blank=True)),
            ],
        ),
        migrations.AddField(
            model_name='shipment',
            name='truck',
            field=models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='search.truck'),
        ),
    ]
