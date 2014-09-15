import urllib
import urllib.parse
import urllib.request
import urllib.error


shaderurl='https://www.shadertoy.com/presets/'
##for i in range(10):
##    f="tex%02i.jpg" % i
##    print(f)
##    try:
##        page = urllib.urlopen(shaderurl+f)
##        data=page.read()
##        page.close()
##        image = open(f, 'wb')
##        image.write(data)
##        image.close()
##    except:
##        print("exception")
        
##for i in range(10):
##    j=i+10
##    f="tex%02i.png" % j
##    print(f)
##    try:
##        page = urllib.urlopen(shaderurl+f)
##        data=page.read()
##        page.close()
##        image = open(f, 'wb')
##        image.write(data)
##        image.close()
##    except:
##        print("exception")

shaderurl='https://www.shadertoy.com/'
#shaderurl='http://www.google.com/'
shaderid="Mss3zM"
#try:
if 1:
    f="{ \"shaders\" : [\""+shaderid+"\"] }"
    f = "s=" + urllib.parse.quote(f);
    #f="shadertoy/?"+f #shaderid
    f="s="+shaderid
    #f="?s="+shaderid
    #f="?s="+shaderid+"&r=1"
    url=shaderurl+"comment"
    print(url+f)
    values={'s':shaderid}
    data = urllib.parse.urlencode(values)
    print(data)
    bdata=f.encode('utf8')
    headers = { 'User-Agent' : 'Opera/9.80 (Windows NT 6.1) Presto/2.12.388 Version/12.15',
                'Host':'www.shadertoy.com' }
    req=urllib.request.Request(url,bdata,headers)
    page=urllib.request.urlopen(req)
    data=page.read()
    page.close()
    print(data)
#except urllib.error.HTTPError as e:
    #print('HTTPError = ' + str(e.code))
