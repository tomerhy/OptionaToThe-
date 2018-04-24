import urllib
import json

from Algo import checkDayTradeInput


class JSONObject:
  def __init__( self, dict ):
      vars(self).update( dict )


url = "http://remote.officecore.com:10005/getSamplesByDay/16042018"

response = urllib.urlopen(url)
jsonobject = json.loads(response.read(), object_hook= JSONObject)

checkDayTradeInput(jsonobject)
#print data


