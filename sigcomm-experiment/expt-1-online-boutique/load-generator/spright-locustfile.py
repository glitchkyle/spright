#!/usr/bin/python
# Based on Locust version 1.6.0

import random, time
from locust import HttpUser, TaskSet, between, runners, events

# Monkey patch stats report interval
runners.WORKER_REPORT_INTERVAL = 1

products = [
    '0PUK6V6EV0',
    '1YMWWN1N4O',
    '2ZYFJ3GM2N',
    '66VCHSJNUP',
    '6E92ZMYYFZ',
    '9SIQT8TOJO',
    'L9ECAV7KIM',
    'LS4PSXUNUM',
    'OLJCESPC7Z']

def index(l):
    l.client.get("/1")

def setCurrency(l):
    currencies = ['EUR', 'USD', 'JPY', 'CAD']
    cur = random.choice(currencies)
    l.client.post("/1/setCurrency" + "?currency_code=" + cur)

def browseProduct(l):
    l.client.get("/1/product?" + random.choice(products))

def viewCart(l):
    l.client.get("/1/cart")

def addToCart(l):
    product = random.choice(products)
    l.client.get("/1/product?" + product)
    l.client.post("/1/cart" + "?" + "product_id=" + product + "&" + "quantity=" + str(random.choice([1,2,3,4,5,10])))

def checkout(l):
    addToCart(l)
    l.client.post("/1/cart/checkout" + "?email=someone@example.com&street_address=1600-Amphitheatre-Parkway&zip_code=94043&city=Mountain-View&state=CA&country=United-States&credit_card_number=4432801561520454&credit_card_expiration_month=1&credit_card_expiration_year=2039&credit_card_cvv=672")

class UserBehavior(TaskSet):

    def on_start(self):
        index(self)
    # tasks = {index: 1}
    tasks = {index: 1,
        setCurrency: 2,
        browseProduct: 10,
        addToCart: 2,
        viewCart: 3,
        checkout: 1}

class WebsiteUser(HttpUser):
    tasks = [UserBehavior]
    wait_time = between(1, 10)

@events.request_success.add_listener
def hook_request_success(request_type, name, response_time, response_length, **kw):
    stats_file.write(str(time.time()) + ";" + request_type + ";" + name + ";" + str(response_time) + "\n")

# @events.request.add_listener
# def hook_request(request_type, name, response_time, response_length, response,
#                  context, exception, **kw):
#     if stats_file:
#         stats_file.write(str(time.time()) + ";" + request_type + ";" + name + ";" + str(response_time) + "\n")

@events.quitting.add_listener
def hook_quitting(environment, **kw):
    if isinstance(environment.runner, runners.WorkerRunner):
        stats_file.close()

@events.init.add_listener
def hook_init(environment, **kw):
    global stats_file
    stats_file = None
    if isinstance(environment.runner, runners.WorkerRunner):
        stats_file = open(f'resp_time_{environment.runner.client_id}.csv', 'w')
