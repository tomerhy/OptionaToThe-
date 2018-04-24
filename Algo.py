import json

def checkDayTradeInput(jsonobject):
    for sample, value in enumerate(jsonobject, 1):
        #print(sample, value)
        checkSample(value)



def  checkSample(value):
    sample = Sample(value.price, value.sampledatestrings, value.strikes)
    sample.printSample()

class Call:
    def __init__(self, base, high, low, change):
        self.base = base
        self.high = high
        self.low = low
        self.change = change

class Put:
    def __init__(self, base, high, low, change):
        self.base = base
        self.high = high
        self.low = low
        self.change = change

class Strike:
    def __init__(self, strike):
        self.strike = strike
        self.call = []
        self.put = []

    def add_call(self, base, high, low, change):
        self.call.append(Call(base, high, low, change))

class Sample:
    def __init__(self, madad_price, sample_date, strikes):
        self.madad_price = madad_price
        self.sample_date = sample_date
        self.strikes = strikes

    def printSample(self):
        print("madad: ", self.madad_price)
        print("date: ", self.sample_date)







