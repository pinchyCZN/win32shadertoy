import urllib
import urllib.parse
import urllib.request

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
shaderid="ldfXzS"
try:
    f="{ \"shaders\" : [\""+shaderid+"\"] }"
    f = "s=" + urllib.parse.quote(f);
    f="shadertoy/?"+f #shaderid
    print(f)
    page=urllib.request.urlopen(shaderurl+f)
    data=page.read()
    page.close()
    print(data)
except:
    print("exception")
