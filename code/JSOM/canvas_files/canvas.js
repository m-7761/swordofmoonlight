
//not actually used to draw
var zfar = 10.0, znear = 0.1;

//TODO: pull defaults from form inputs

var canvas = //basically a namespace 
{            
 debug : false, //set in debugger
 shadow : true, //on by default now
 
 start : '', //prefix for outgoing urls
 
 display : false, //hidden at start
 
 radius : 7.0, //radius of current scene
 pivot : null, //center of radius
 bbox : null, //matrix
   
 scale : 1, pan : [0,0], 
  
 viewport : [0,0,0,0],
	
	//60/25: hardcoded for SOM objects
	//scroll states of the 5 4 3 2 1 radios
	d : [3,,,['60','25'],['100','75']],
	
	dl : [,[50,50],[50,50],[60,25],[100,75],[50,50]],

 speed : 1, //play speed
 
 play : false, loop : false, mute : true,
  
 tracks : [], t : 0, //track#
  
 identity : mat4.create(mat4.identity()),
   
 //horizontal rotation
 convention : [-1,1,0],
 //could just be a single angle?
 rotate : vec3.create([-1,1,0]),
 
 light : quat4.create([0,7,-4,7]), 
   	
	requests : [], 
	inputs : [],
	
	gl : null, //WebGL context
	
	composite : 0, //TODO: compartmentalize...
  
 min : 1, max : Number.MAX_VALUE, //priority
	 
  //recomposition
 //
 trackpos : [], //save track times 
 textures : [], //save textures 
 	
	program : null, //apply3D/initProgram
	
	programs : [],  	
	buffers : [],
	passes : [],
	forms : [],
	keys : [],

 n : 0, frame : 0, //animation
 	
	//2 does not help Chrome since AA is default
	//FF10 is said to have AA (may not be default)
	//TODO: 2 should be default but prefer AA
 //1 should disable AA. webGLSuper(2) API 
	x : 1, //2x supersampling
};

canvas.cleanup = function()
{
 canvas.inputs.length = 0;
	canvas.requests.length = 0;
}

canvas.repair = 
canvas.prepare = function()
{
 canvas.buffers.length = 0;
 canvas.passes.length = 0;
 canvas.forms.length = 0;
 canvas.keys.length = 0; 
}
       
//addEvent function from 
//http://www.quirksmode.org/blog/archives/2005/10/_and_the_winner_1.html
canvas.addEvent = function(obj, type, fn) 
{
	if (obj.addEventListener)
		obj.addEventListener(type, fn, false);
	else if (obj.attachEvent) {
		obj["e"+type+fn] = fn;
		obj[type+fn] = function() { obj["e"+type+fn]( window.event ); }
		obj.attachEvent("on"+type, obj[type+fn]);
	}
}
	
canvas.removeEvent = function( obj, type, fn )
{
	if (obj.removeEventListener)
		obj.removeEventListener( type, fn, false );
	else if (obj.detachEvent)
	{
		obj.detachEvent( "on"+type, obj[type+fn] );
		obj[type+fn] = null;
		obj["e"+type+fn] = null;
	}
}	

//lifted from Underscore.js library
canvas.throttle = function(func, wait) 
{
 var timeout;
 return function()
 {
  var context = this, args = arguments;
  
  if(!timeout) 
  {
   // the first time the event fires, we setup a timer, which 
   // is used as a guard to block subsequent calls; once the 
   // timer's handler fires, we reset it and create a new one
   timeout = setTimeout(function() 
   {
    timeout = null; func.apply(context, args);
   },wait);
  }
 }
}

canvas.needElement = function()
{
 if(canvas.element) return canvas.element;
 
 var canvases = document.getElementsByTagName("canvas");

 if(!canvases||canvases.length===0)
 {
  alert("canvas.js: canvas is absent."); return;
 }

 canvas.element = canvases.item(0);
  
 if(canvases.length>1)
 {
  alert("canvas.js: more than one canvas. There can be only one.");    
 }
  
 //unfortunately this is the only way to resize
 //TODO: do not run in background (when out of focus)
 
 canvas.lastWidth = canvas.element.clientWidth;
 canvas.lastHeight = canvas.element.clientHeight;
  
 setInterval(function() 
 {
  if(canvas.lastWidth!==canvas.element.clientWidth
   ||canvas.lastHeight!==canvas.element.clientHeight)
  {
   canvas.lastHeight = canvas.element.clientHeight;
   canvas.lastWidth = canvas.element.clientWidth;   
   canvas.view3D(); canvas.present3D();   
  }
 },300);
 
 return canvas.element;
}

canvas.refresh = function(n)
{
 var d = n?n:canvas.d[0];
  
 switch(d)
 {
 //1: view3D is just for Z roll
 //Would prefer only the crop3D
 case 1: canvas.view3D();  
         canvas.crop3D(); break; //1D
 case 2: canvas.crop3D(); break; //2D 
 case 3: canvas.view3D(); break; //3D 
 case 4: canvas.play3D(); break; //4D
 }
 
 //play3D will refresh itself
 if(d!=4) canvas.refresh3D();
}

canvas.js_init = function()
{
 var xy = document.getElementById('canvas-ctrl-xy');
 
 //0 is the bottom, 100 is at the top. 
 canvas.addEvent(xy,'scroll',function(e)
 {
  e = e||event; //crazy non-boolean ||... 
	
	 var target = e.srcElement||e.target;

  //scrollHeight/Width: Chrome complicates by substracting
  //scrollbar width (but not height!) in figuring a percentage.
  var w = target.scrollWidth-target.clientWidth;
  var h = target.scrollHeight-target.clientHeight;
	  
  var x = document.getElementById('canvas-ctrl-x');
  var y = document.getElementById('canvas-ctrl-y');
  
  var x_val = 100-Math.round(target.scrollLeft/w*100);
  var y_val = 100-Math.round(target.scrollTop/h*100)
  
  if(x&&document.activeElement!=x) x.value = x_val;
  if(y&&document.activeElement!=y) y.value = y_val;
  
  if(canvas.d[0]===4)
  {   
   canvas.speed = (y_val-50)*2/50;
  
   if(!canvas.playing) canvas.trackTime(1-x_val/100);
  }
    
  //update current values/present
  canvas.js_d(); canvas.refresh(); 
 });
   
 var move = document.getElementById('canvas-ctrl-move');
 
 canvas.addEvent(move,'mousedown',function(e) 
 {
  if(canvas.moveZ) return; //already down
  
  var f, g; e = e||event; canvas.moveZ = true;
  
  canvas.moveX = e.clientX; canvas.moveY = e.clientY;
  
  canvas.addEvent(document,'mousemove',f=function(e)
  {
   xy.scrollLeft+=e.clientX-canvas.moveX; //dX	 
	  xy.scrollTop+=e.clientY-canvas.moveY; //dY
	  
   canvas.moveX = e.clientX; canvas.moveY = e.clientY;   
  });
  
  canvas.addEvent(document,'mouseup',g=function(e)
  {
   canvas.removeEvent(document,'mousemove',f);
   canvas.removeEvent(document,'mouseup',g);   
   
   canvas.moveZ = false;
  });
	
	e.preventDefault(); 
  
 }); 
}

canvas.js_xy = function(x,val,val2)
{
 //0 is the bottom, 100 is at the top.
 var n = parseInt(val,10), t = (100-n)/100;
 
 var xy = document.getElementById('canvas-ctrl-xy');
 
 if(!xy||isNaN(n)||n<0||n>100) return;
   
 switch(x) //scrollHeight/Width: Chrome complicates by substracting
 {       //scrollbar width (but not height!) in figuring a percentage.
 case 'xy':
 case 'x': xy.scrollLeft = Math.round((xy.scrollWidth-xy.clientWidth)*t); break;   
 case 'y': xy.scrollTop = Math.round((xy.scrollHeight-xy.clientHeight)*t); break;
 } 
 
 if(x=='xy') canvas.js_xy('y',val2);
}

canvas.js_X = function()
{
 document.body.className = 'x';
 
 var pal = parent.document.getElementById('canvas-pal');
   
 if(pal) pal.style.visibility = 'hidden';
}

canvas.js_x = function(x)
{
 if(x==1||x==2)
 {
  canvas.x = x; canvas.present3D();
 }
 else alert('I think you meant canvas.js_X()!');
}

canvas.js_v = function(e)
{
 if(document.body.className==='v') 
 {
  //New: shadow hack 
  {
   e = e||event; 
 	
 	 var target = e.srcElement||e.target;
 	 
 	 if(target==canvas.element
   ||target.id==='canvas-ctrl-1-5'
   ||target.id==='canvas-ctrl-rec')
   {
    canvas.shadow = !canvas.shadow;
    canvas.present3D();
   } 
 	}
	 
  return;
 }
  
 //NOTE: if controls are invisible layout is undefined 
 
 document.body.className = 'v'; //reveal controls
 
 var x = document.getElementById('canvas-ctrl-x');
 var y = document.getElementById('canvas-ctrl-y');
 
 if(x) canvas.js_xy('x',x.value);
 if(y) canvas.js_xy('y',y.value);
  
 var pal = parent.document.getElementById('canvas-pal');
   
 if(pal) pal.style.visibility = 'visible';
}

canvas.rec = function(o)
{ 
  var xy = document.getElementById('canvas-ctrl-1-5');
    
  if(!xy||xy.style.display==(o?'none':'')) return o;
  
  var rec = document.getElementById('canvas-ctrl-rec');
  
  if(rec&&xy)
  {
   rec.style.display = o?'':'none'; 
   xy.style.display = o?'none':'';
  }      
  
  return o;
}

canvas.js_d = function(n)
{
 var m = canvas.d[0]; //current
 
 if(!canvas.display) return;
  
 var x = document.getElementById('canvas-ctrl-x');
 var y = document.getElementById('canvas-ctrl-y');
   
 if(x) canvas.d[m][0] = x.value; //save x 
 if(y) canvas.d[m][1] = y.value; //save y
 
 if(!n||canvas.rec(n==='rec')) return;
 
 if(!canvas.d[n])
 {
  canvas.d[n] = ['50','50']; //defaults
 }
 
 if(x) x.value = canvas.d[n][0]; //load x
 if(y) y.value = canvas.d[n][1]; //load y
 
 if(x) x.onchange(); if(y) y.onchange();
 
 canvas.d[0] = n;
}

canvas.js_dl = function(n)
{
 if(canvas.d[0]!=n)
 {
  canvas.d[n] = [String(canvas.dl[n][0]),String(canvas.dl[n][1])];
 }
 else canvas.js_xy('xy',canvas.dl[n][0],canvas.dl[n][1]); 
 
 canvas.refresh(n);
}

canvas.radio = function(name)
{
 var radios = document.getElementsByName(name);
 
 for(var i=0;i<radios.length;i++) if(radios[i].checked) return radios[i].value;
 
 return undefined;
}

canvas.js_play = function(play)
{
 canvas.changeTrack(canvas.radio('canvas-ctrl-t'));
 
 canvas.loop = document.getElementById("canvas-ctrl-loop").checked;
 canvas.play = document.getElementById("canvas-ctrl-play").checked;
 canvas.mute = document.getElementById("canvas-ctrl-mute").checked;
 
 canvas.play3D();
}

canvas.removeTracks = function()
{
 var tracks = document.getElementById('canvas-ctrl-tracks'); 
 var radios = tracks.getElementsByTagName('label');
 
 while(radios.length>1) tracks.removeChild(radios.item(radios.length-1));
  
 for(var i=0;i<canvas.tracks.length;i++)
 {
  var track = canvas.tracks[i];
  
  if(track.time!==undefined) //record/restore
  {
   canvas.trackpos[track.pos] = track.time;
  }
 } 
}

canvas.addTracks = function(add)
{
 if(!add) //simply replace tracks 
 {
  add = canvas.tracks; canvas.removeTracks(); //paranoia
 }
    
 var tracks = document.getElementById('canvas-ctrl-tracks');
 
 var m = add.length, n = tracks.getElementsByTagName('label').length-1;
 
 for(var i=0,value=n;i<m;i++,value++) if(value!==0)
 {
  var loading = 'class="track"';    
  var title = add[i].number+': '+(add[i].item.title||'untitled');
  
  if(canvas.composing&&!add[i].key.length)
  {
   loading = 'class="track loading" disabled="disabled"';   
   title+=' (loading...)';
  }
  
  var checked = value==canvas.t?'checked="checked"':'';
    
  var label = value>8?String.fromCharCode(88+value):value+1;
  
  tracks.innerHTML+=
  ' <label '+loading+' title="'+title+'">'+label+
  ' <input id="canvas-ctrl-track-'+value+'" name="canvas-ctrl-t" '+checked+
  '        type="radio" value="'+value+'" onclick="javascript:'+
  ' canvas.js_play()"></label> ';
 }
}

canvas.changeTrack = function(to)
{
 var t = Number(to); 
 
 var current = canvas.tracks[canvas.t];
 
 if(canvas.tracks.length)
 { 
  t = (canvas.tracks.length+t)%canvas.tracks.length;
 }
 else t = 0; if(canvas.t===t) return;
 
 if(current) current.pause = true;
 
 var change = canvas.tracks[canvas.t=t];
 
 //hack: prefer rotation animation to loading animation
 if(!change.key.length) change = canvas.tracks[canvas.t=t=0];
 
 var time = change.time?change.time/change.run:0;
 
 if(canvas.speed<0&&!change.time) time = 1;
   
 canvas.d[4][0] = String(100-Math.round(time*100));
 if(canvas.d[0]===4) canvas.js_xy('x',canvas.d[4][0]);
 
 if(!change.rot) vec3.set(canvas.convention,canvas.rotate);
 
 var radio = document.getElementById('canvas-ctrl-track-'+t);
 
 if(radio) radio.checked = true;
}

canvas.trackTime = function(set)
{
 var ct = canvas.tracks[canvas.t];
 
 if(!ct||ct.time===undefined) return 0;
 
 if(set!==undefined)
 {
  if(set<=0.0003) ct.time = 0;
  
  else if(set>=0.9997) ct.time = ct.run;
  
  else ct.time = ct.run*set%ct.run;
 }
 
 return ct.time/ct.run;
}

canvas.readyTrack = function(track)
{  
 if(!track.key.length)
 { 
  track.min = 0;  
  track.max = track.rot?6:0;    
 }
 else
 {   
  track.min = +Number.MAX_VALUE;
  track.max = -Number.MAX_VALUE;
 }
 
 for(var i=0;i<track.key.length;i++)
 {
  var key = track.key[i]; 
  
  if(key.min===undefined)
  {  
   var group = key.group, frames = key.frames;
    
   var min = +Number.MAX_VALUE, max = -min;
   
   if(group.timeframe)
   {
    min = group.timeframe[0];    
    max = group.timeframe[1];
   }
   else for(var i=0;i<frames.length;i++)
   {
    min = Math.min(frames[i].timerange[0],min);
    max = Math.max(frames[i].timerange[1],max);
   }
   
   if(isNaN(min)||min===+Number.MAX_VALUE) min = 0; 
   if(isNaN(max)||max===-Number.MAX_VALUE) max = 0;   
        
   key.speed = group.timescale || 1;
   
   key.min = min; key.max = max;    
  } 
  track.min = Math.min(key.min*key.speed,track.min);
  track.max = Math.max(key.max*key.speed,track.max);    
 }
 track.run = track.max-track.min;
    
 track.still = track.run===0;   
 track.pause = true;
   
 //we save track times in trackpos so 
 //that they may survive recomposition.
 if(canvas.trackpos[track.pos]!==undefined)
 {
  track.time = canvas.trackpos[track.pos]%track.run;
 }
 else canvas.trackpos[track.pos] = 0;
 
 track.time = canvas.trackpos[track.pos];
}

canvas.readyTexture = function(src) 
{ 
 if(!canvas.textures[src]) 
 {  
  var out = new Image();
  
  out.loaded = false;  
  out.onload = function() 
  {
   canvas.readyTextureComplete(out);
  };
  
  out.onerror = function() 
  {
   setTimeout(function()
   {  
    if(out.errors++<5)
    {
     var src = out.src; out.src = ""; out.src = src;
    }
    else out.onerror = null;
   },100);
  };
  
  if(src.substr(0,5)!='data:')
  {
   out.src = canvas.start+src; //breakpoint 
	}
	else out.src = src;
   
 // if(out.complete)
 // {
 //  //file:/// onload never has a chance??? 
 //  canvas.readyTextureComplete(out);
 // }
 
  canvas.textures[src] = out;
 }
 
 return canvas.textures[src];
}

canvas.readyTextureComplete = function(img)
{
 img.loaded = img.complete;
 
 if(!img.src)
 {
  img.loaded = false
  
  if(img.tex2D) img.tex2D = canvas.gl.deleteTexture();
 }
 else if(!img.tex2D||img.src2D!=img.src)
 {
  img.src2D = String(img.src);
  img.tex2D = canvas.gl.createTexture();
   
  var gl = canvas.gl; 
  gl.bindTexture(gl.TEXTURE_2D,img.tex2D);
  //TODO: need a flip input or something...   
  //gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL,true);
  gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,img);
  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);
  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR_MIPMAP_NEAREST);
  //gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.GL_LINEAR_MIPMAP_LINEAR);
  //TEXTURE_MAX_ANISOTROPY_EXT ??
  
  gl.generateMipmap(gl.TEXTURE_2D);    
  gl.bindTexture(gl.TEXTURE_2D,null);
 }
 
 if(img.loaded) canvas.present3D();
}

canvas.initGL = function() 
{        
 if(canvas.gl) return; //been here, done that 
 
 try 
 {
  canvas.gl = 
  canvas.element.getContext("experimental-webgl",{alpha:false});
 } 
 catch(e)
 {
 }
 if(!canvas.gl) 
 {
     //alert("Could not initialise WebGL, sorry :-(");
     
     document.getElementById('canvas-sans-webgl').style.display = '';
 }
}

//scheduled obsolete
//m and v are just for testing
canvas.mMatrix = mat4.create();
canvas.vMatrix = mat4.create();
canvas.pMatrix = mat4.create();
canvas.mvMatrix = mat4.create();
//mvp is combined automatically
canvas.mvpMatrix = mat4.create();
canvas.texMatrix = mat3.create();
//gl-matrix.js does not have a vec4 array
//(Float32Array types do not seem to be expand)
canvas.diffuseCol = quat4.create([1,1,1,1]);
canvas.emissiveCol = quat4.create([0,0,0,0]);
canvas.colorKey = quat4.create([0,0,0,-2]); //none
canvas.backFace = quat4.create([0,0,0,1]); //black
canvas.setProgramUniforms = function(of) //eg. 'demo' 
{
 var prog = typeof of=="object"?of:canvas.programs[of];
 
 if(prog.pMatrixUniform)
 canvas.gl.uniformMatrix4fv(prog.pMatrixUniform,false,canvas.pMatrix);
 if(prog.mvMatrixUniform)
 canvas.gl.uniformMatrix4fv(prog.mvMatrixUniform,false,canvas.mvMatrix);
 if(prog.mvpMatrixUniform)
 {
  mat4.multiply(canvas.pMatrix,canvas.mvMatrix,canvas.mvpMatrix);
  canvas.gl.uniformMatrix4fv(prog.mvpMatrixUniform,false,canvas.mvpMatrix);
 }
 if(prog.texMatrixUniform)
 canvas.gl.uniformMatrix3fv(prog.texMatrixUniform,false,canvas.texMatrix);
 if(prog.diffuseColUniform)
 canvas.gl.uniform4fv(prog.diffuseColUniform,canvas.diffuseCol);
 if(prog.emissiveColUniform)
 canvas.gl.uniform4fv(prog.emissiveColUniform,canvas.emissiveCol);
 if(prog.colorKeyUniform) 
 canvas.gl.uniform4fv(prog.colorKeyUniform,canvas.colorKey);  
 
 if(prog.lightUniform) 
 canvas.gl.uniform4fv(prog.lightUniform,canvas.light);
 if(prog.rotateUniform) 
 canvas.gl.uniform3fv(prog.rotateUniform,canvas.rotate);
 if(prog.stateUniform)
 canvas.gl.uniformMatrix4fv(prog.stateUniform,false,canvas.identity);
}
 
//scheduled obsolete
canvas.getShader = function(id,D) 
{    
    var shaderScript = document.getElementById(id);
    
    if(!shaderScript) return null;
        
    var str =
    "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"+
    "precision highp float;\n"+
    "#else\n"+
    "precision mediump float;\n"+
    "#endif\n";
    
    if(id!="demo") //hack
    if(shaderScript.type==="x-shader/x-vertex")
    {    
     if(D) str+=D;    
     
     var define = function(x,y)
     {
       //FF spits up an alert box on redefinition...
      return "#ifndef "+x+"\n#define "+x+y+"\n#endif\n";
     };
         
     str+= //default semantics
     "attribute vec3 position;\n"+
     define('POSITION',' vec4(position,1)')+
     define('NORMAL',' vec4(normal,0)')+
     define('TEXCOORD0',' vec3(texcoord0,1)')+
     "attribute vec3 tweenpos;\n"+
     "uniform float tween;\n"+
     "uniform vec3 rotate;\n"+ //float
     define('ROTATE','(V) '+ //vec3 rotate (cos,1,sin) 
     //might make more sense to just use a single angle 
     'V.xyzw*rotate.xyxy+vec4(-1,0,+1,0)*V.zyxw*rotate.zyzy')+
     define('WEIGHT',' vec4(1.0,1.0,1.0,1.0)')+
     define('STATES',' 1')+
     "uniform mat4 state[4];\n"+     
     define('STATE0','(X) state[0]*X')+
     define('STATE1','(X) X')+ //TODO: state[1]*X
     define('STATE2','(X) X')+ //TODO: state[2]*X
     define('STATE3','(X) X'); //TODO: state[3]*X    
    }        

    var k = shaderScript.firstChild;
    
    while(k) 
    {
      if(k.nodeType==3) str+=k.textContent;
      
      k = k.nextSibling;
    }

    var shader;
    if(shaderScript.type==="x-shader/x-fragment") 
    {
        shader = canvas.gl.createShader(canvas.gl.FRAGMENT_SHADER);
    }
    else if(shaderScript.type==="x-shader/x-vertex") 
    {
        shader = canvas.gl.createShader(canvas.gl.VERTEX_SHADER);        
    }
    else return null;

    canvas.gl.shaderSource(shader,str);
    canvas.gl.compileShader(shader);

    if(!canvas.gl.getShaderParameter(shader, canvas.gl.COMPILE_STATUS)) 
    {
        alert(canvas.gl.getShaderInfoLog(shader)); return null;
    }

    return shader;
}

//scheduled obsolete
canvas.initProgram = function(name,D)
{    
  var id = D?name+'... '+D:name;
  
  if(canvas.program=canvas.programs[id]) return false;
  
//Note: D should eventually be individualized per shader
   
  var fs = canvas.getShader(name+'-fs',D);
  var vs = canvas.getShader(name+'-vs',D);

  var prog = canvas.gl.createProgram();
  
  canvas.gl.attachShader(prog,fs);
  canvas.gl.attachShader(prog,vs);
  canvas.gl.linkProgram(prog);

  if(!canvas.gl.getProgramParameter(prog,canvas.gl.LINK_STATUS)) 
  {
    if(D) //attempt fallback to default
    {
     canvas.initProgram(name);     
     if(canvas.programs[id]=canvas.program) 
     {
      return canvas.programs[name]?false:true;
     }
    } 
    
    alert('Could not initialise '+name+' program');
    
    return false;
  }

  prog.pMatrixUniform = 
  canvas.gl.getUniformLocation(prog,"uPMatrix");
  prog.mvMatrixUniform = 
  canvas.gl.getUniformLocation(prog,"uMVMatrix");
  prog.mvpMatrixUniform = 
  canvas.gl.getUniformLocation(prog,"uMVPMatrix");    
  prog.texMatrixUniform = 
  canvas.gl.getUniformLocation(prog,"uTexMatrix");
  
  prog.rotateUniform =
  canvas.gl.getUniformLocation(prog,"rotate");
  prog.opacityUniform =
  canvas.gl.getUniformLocation(prog,"opacity");
  prog.tweenUniform =
  canvas.gl.getUniformLocation(prog,"tween"); 
  prog.lightUniform =
  canvas.gl.getUniformLocation(prog,"light");
  prog.stateUniform =
  canvas.gl.getUniformLocation(prog,"state"); 
  
  canvas.gl.useProgram(prog); //assuming not problematic
  
  if(prog.stateUniform)
  canvas.gl.uniformMatrix4fv(prog.stateUniform,false,canvas.identity)
    
  if(prog.texMatrixUniform)
  canvas.gl.uniformMatrix3fv(prog.texMatrixUniform,false,canvas.texMatrix)
  
  canvas.programs[prog.id=id] = prog;
  
  canvas.program = prog;
  
  return true;
}

//scheduled obsolete
canvas.programSemantics = function(prog,semantics)
{
 if(!prog.semantics) prog.semantics = [];
 
 for(var i=0;i<semantics.length;i++)
 {
  var j = semantics[i];
  var p = prog.semantics[j] = canvas.gl.getAttribLocation(prog,j);   
  if(p<0) prog.semantics[j] = undefined;
  
  //Apparently everything is enabled by default
  if(p>=0) canvas.gl.disableVertexAttribArray(p);
 }
 
 //Assuming semantics are attributes for now
 prog.attributes = prog.semantics;
} 

canvas.disablePrograms = function()
{
 for(var i in canvas.programs)
 {
  var p = canvas.programs[i], at = p.attributes;
  
  canvas.gl.useProgram(p);  
  for(var j in at) if((p=at[j])>=0)
  { 
   canvas.gl.disableVertexAttribArray(p);
  }
 } 
}

//scheduled obsolete 
canvas.needDefault = function(D)
{
 if(canvas.initProgram('default',D))
 {
  var prog = canvas.program;  
   
  prog.diffuseColUniform =
  canvas.gl.getUniformLocation(prog,"uColDiffuse");
  prog.emissiveColUniform =
  canvas.gl.getUniformLocation(prog,"uColEmissive");
  prog.colorKeyUniform =
  canvas.gl.getUniformLocation(prog,"uColorKey");
    
  canvas.programSemantics(prog,
  ['position','tweenpos','normal','texcoord0']);
  
  return prog; 
 }
 
 return canvas.program;
}

//scheduled obsolete
canvas.needShadow = function(D)
{
 if(canvas.initProgram('shadow',D))
 {
  var prog = canvas.program;
  
  prog.shadowMatUniform =
  canvas.gl.getUniformLocation(prog,"uShadowMat");    
  prog.shadowColUniform =
  canvas.gl.getUniformLocation(prog,"uShadowCol");
  prog.fogColUniform =
  canvas.gl.getUniformLocation(prog,"uFogCol");
  prog.fogRadiusUniform =
  canvas.gl.getUniformLocation(prog,"uFogRadius");
  
  canvas.programSemantics(prog,
  ['position','tweenpos']); 
 }
 
 return canvas.program;
}

//scheduled obsolete
canvas.needSolid = function(D)
{ 
 if(canvas.initProgram('solid',D))
 {
  var prog = canvas.program;
  
  prog.solidColUniform =
  canvas.gl.getUniformLocation(prog,"uSolidCol");
    
  canvas.programSemantics(prog, 
  ['position','tweenpos','texcoord0']); 
 }
 
 return canvas.program;
}

//scheduled obsolete
canvas.needDebug = function()
{
 canvas.initDemo(); //TODO 'debug'
 
 return canvas.programs['demo'];
} 

canvas.initDemo = function() 
{    
    if(canvas.programs['demo']) return;
    
    canvas.initProgram('demo');
    
    var demo = canvas.programs['demo'];
    
    canvas.gl.useProgram(demo);

    demo.position = canvas.gl.getAttribLocation(demo,"aVertexPosition");
    canvas.gl.enableVertexAttribArray(demo.position);

		  demo.color = canvas.gl.getAttribLocation(demo,"aVertexColor");
    canvas.gl.enableVertexAttribArray(demo.color);
    
    ////////////////////////////////
    
    canvas.demoTriangle = canvas.gl.createBuffer();
    canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER,canvas.demoTriangle);
    var vertices = [
         0.0,  1.0,  0.0,
        -1.0, -1.0,  0.0,
         1.0, -1.0,  0.0
    ];
    canvas.gl.bufferData(canvas.gl.ARRAY_BUFFER,new Float32Array(vertices),canvas.gl.STATIC_DRAW);
    canvas.demoTriangle.itemSize = 3;
    canvas.demoTriangle.numItems = 3;

		  canvas.demoTricolor = canvas.gl.createBuffer();
    canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER,canvas.demoTricolor);
    var colors = [
        1.0, 0.0, 0.0, 1.0,
        0.0, 1.0, 0.0, 1.0,
        0.0, 0.0, 1.0, 1.0
    ];
    canvas.gl.bufferData(canvas.gl.ARRAY_BUFFER,new Float32Array(colors),canvas.gl.STATIC_DRAW);
    canvas.demoTricolor.itemSize = 4;
    canvas.demoTricolor.numItems = 3;
    
    canvas.demoSquare = canvas.gl.createBuffer();
    canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER,canvas.demoSquare);
    vertices = [
         1.0,  1.0,  0.0,
        -1.0,  1.0,  0.0,
        -1.0, -1.0,  0.0,
         1.0, -1.0,  0.0,
    ];
    canvas.gl.bufferData(canvas.gl.ARRAY_BUFFER, new Float32Array(vertices), canvas.gl.STATIC_DRAW);
    canvas.demoSquare.itemSize = 3;
    canvas.demoSquare.numItems = 4;
    
    canvas.demoLavender = canvas.gl.createBuffer();
    canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER, canvas.demoLavender);
    
    colors = []
    for (var i=0; i < 4; i++){ colors = colors.concat([0.5, 0.5, 1.0, 1.0]); }
    canvas.gl.bufferData(canvas.gl.ARRAY_BUFFER, new Float32Array(colors), canvas.gl.STATIC_DRAW);
    canvas.demoLavender.itemSize = 4;
    canvas.demoLavender.numItems = 4;
}

canvas.drawDemo = function() 
{        
    canvas.gl.viewport(0,0,canvas.element.width,canvas.element.height);
    canvas.gl.clear(canvas.gl.COLOR_BUFFER_BIT|canvas.gl.DEPTH_BUFFER_BIT);
    
    mat4.perspective(45,canvas.element.width/canvas.element.height,znear,zfar,canvas.pMatrix);

    mat4.identity(canvas.mvMatrix);

    mat4.translate(canvas.mvMatrix,[-0.5, -0.5, -7.0]);
    canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER, canvas.demoTriangle);
    canvas.gl.vertexAttribPointer(canvas.programs['demo'].position, canvas.demoTriangle.itemSize, canvas.gl.FLOAT, false, 0, 0);
            
    canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER, canvas.demoTricolor);
    canvas.gl.vertexAttribPointer(canvas.programs['demo'].color, canvas.demoTricolor.itemSize, canvas.gl.FLOAT, false, 0, 0);

		  canvas.setProgramUniforms('demo'); //TRIANGLES
    canvas.gl.drawArrays(canvas.gl.LINE_LOOP, 0, canvas.demoTriangle.numItems);

    mat4.translate(canvas.mvMatrix,[3.0, 0.0, 0.0]);
    canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER, canvas.demoSquare);
    canvas.gl.vertexAttribPointer(canvas.programs['demo'].position, canvas.demoSquare.itemSize, canvas.gl.FLOAT, false, 0, 0);
            
    canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER, canvas.demoLavender);
    canvas.gl.vertexAttribPointer(canvas.programs['demo'].color, canvas.demoLavender.itemSize, canvas.gl.FLOAT, false, 0, 0);

    canvas.setProgramUniforms('demo'); //TRIANGLE_FAN
    canvas.gl.drawArrays(canvas.gl.LINE_LOOP, 0, canvas.demoSquare.numItems);
}

//TODO:consider jsomStart
function webGLStart(base) 
{
  var iframe = 
  parent.document.getElementById(window.name);
   
  //Assuming a dup'd canvas due to cloneNode effect
  if(iframe&&iframe.contentWindow!=self) return; 
   
  if(!base) base = "";
  
  if(base==="demo")
  {
   webGLStart.demo = true;
  }
  else if(canvas.start!=base)
  {   
   canvas.cleanup(); //begin anew from scratch
   
   canvas.start = base;
  }

  canvas.element = canvas.needElement();
 
  if(!canvas.element) return;
       
  if(!canvas.gl)
  {
   canvas.initGL(); if(!canvas.gl) return;
     
   if(webGLStart.demo) canvas.initDemo(); 
  }
  
  canvas.prepare();

  mat3.identity(canvas.texMatrix);
    
  //culling:
  //should be per side (and setting)
  canvas.gl.enable(canvas.gl.CULL_FACE);  
  canvas.gl.cullFace(canvas.gl.BACK);
 
  canvas.gl.clearColor(0,12/255,12/255,1); //hack: hardcoding #000C0C
  canvas.gl.clear(canvas.gl.COLOR_BUFFER_BIT|canvas.gl.DEPTH_BUFFER_BIT);
    
  canvas.gl.enable(canvas.gl.DEPTH_TEST);
  canvas.gl.depthFunc(canvas.gl.LEQUAL);

  if(webGLStart.demo) canvas.drawDemo();
    
  if(self!=top) setTimeout(function()
		{
		 //Browsers will screw with the scrollbars on hide/show
		 var x = document.getElementById('canvas-ctrl-x').value;
   var y = document.getElementById('canvas-ctrl-y').value;
       
		 iframe.style.display = 'block';				  
		 
		 document.getElementById('canvas-ctrl-x').value = x;
		 document.getElementById('canvas-ctrl-y').value = y;
		 		 
		 if(document.body.className==='v') 
   {
    canvas.js_xy('xy',x,y); //repair scrollbars
   }
      
   var pal = parent.document.getElementById('canvas-pal');
   
   if(pal) pal.style.display = '';
  
		},500);
		
		canvas.display = true; //assuming
}

function webGLStop() 
{
 if(!canvas.gl) return;
 
 //TODO: unload any resources if need be
  
 if(self!=top)
 {
  //Browsers will screw with the scrollbars on hide/show
  var x = document.getElementById('canvas-ctrl-x').value;
  var y = document.getElementById('canvas-ctrl-y').value;
   
  var iframe = parent.document.getElementById(window.name);        
  
	 iframe.style.display = 'none'; canvas.removeTracks();
	 
	 document.getElementById('canvas-ctrl-x').value = x;
		document.getElementById('canvas-ctrl-y').value = y;
				
  var pal = parent.document.getElementById('canvas-pal');
  
  if(pal) pal.style.display = 'none';		
	}      
	
	canvas.display = false;
	
	canvas.repair();
}

function webGLRequest(name,url,type)
{
 if(!canvas.gl) return;
  
 //TODO: if scale is zero pause requests somehow
 
 if(name)
 canvas.inputs[name] = url?canvas.requests[url]:null; 
 
 if(!type) return; //TODO: try url extension
 
 if(!url||canvas.requests[url]) return; 

 canvas.requests[url] = {url:url,type:type,missing:true,content:{}};
 
 if(name) canvas.inputs[name] = canvas.requests[url];  
  
 if(type==='json') webGLInternalRequestJSON(url);
 if(type==='png') webGLInternalRequestURL(url,type,'images'); 
}

//A webGL API like name should not be used here.
function webGLInternalRequestJSON(url)
{
 //getJSON: jQuery 1.4x cannot do errors  
 jQuery.getJSON(canvas.start+url,function(parsed_json_object) 
 {
  webGLInternalRequestJSONSuccess(url,parsed_json_object);
 });
}

//A webGL API like name should not be used here.
function webGLInternalRequestJSONSuccess(url,json)
{
 if(json.externals)  
 {  
  for(var i=0;i<json.externals.length;i++)
  {
   var ext = json.externals[i];   
   ext.priority = ext.priority||1;  
   if(ext.json) webGLRequest(0,ext.json,ext.type='json');
   //Hmmm: I am unclear if this next line should be???
   if(ext.png) webGLRequest(0,ext.png,ext.type='png');     
  }
 }
  
 for(var i in json) //hack: none of this descends!   
 {
  var a = json[i], j, aj, aN = a.length;
      
  if(json.externals)
  for(aj=a[j=0];j<aN;aj=a[++j]) if(aj.url!==undefined) 
  {    
   var ext = json.externals[aj.ext=aj.url];
   //this way the context is unnecessary for lookup 
   aj.type = ext.type; aj.url = ext[aj.type];   
   aj.pri = ext.priority;
   
   //HACK:
   //trying to get compose3D to use the image 
   //item itself vs the raw external PNG request. 
   //Because it can have more info, eg. alpha/scale
   if(aj.type!='json') aj.inf = true; 
    
   //TODO: need a way to detect circular JSOM files
   
   //Thoughts: could just cap depth if need be.
  }   
  
  //TODO: template expansion
        
  if(i==='keyframes') for(aj=a[j=0];j<aN;aj=a[++j])   
  {
   if(aj.graphics) aj.graphics = 
   canvas.collect(json,'templates',aj.graphics,[]);
  }      
 }
  
 json.request = canvas.requests[url]; //two
 canvas.requests[url].content = json; //way
 canvas.requests[url].missing = false;
 
 //Add low priority content to composition
 if(canvas.composing) canvas.compose3D();
}

//A webGL API like name should not be used here. 
function webGLInternalRequestURL(url,type,of)
{ 
 //create a dummy context.
 //Works well because all links converge on the dummy...
 //Should guarantee no duplicate instances of url.
 
 //{externals:[{<type>:url}],<of>:[{'url':0}]};
 
 var content =  {externals:[{}]};
 
 canvas.requests[url].content = content; //two 
 content.request = canvas.requests[url]; //way
  
 content.externals[0][type] = url; //png:"http..."
 
 //rearrange same as webGLInternalRequestJSONSuccess  
 content[of] = [{url:url,type:type,ext:0,inf:true,pri:0}]; 
 
 content.request.infinite = true; //inf: circular
 
 canvas.requests[url].missing = false; 
}

function webGLDisplay(name,min,max)
{
 if(!canvas.gl) return;
 
 for(i in canvas.inputs)
 {
  canvas.inputs[i].display = false;
 }
 
 if(name&&!canvas.inputs[name]) return;
 
 canvas.inputs[name].display = true;
 
 webGLStart.demo = false; //hack
 
 //Assuming single compartment...
 canvas.max = max||Number.MAX_VALUE; 
 canvas.min = min||1; 
 
 canvas.trackpos.length = 0; //hacks... 
 for(t in canvas.textures) delete canvas.textures[t];
 canvas.disablePrograms(); 
 
 canvas.compose3D(); 
}

function webGLRefresh()
{ 
 if(!canvas.gl) return;
 
 if(webGLStart.demo&&!content.length) canvas.drawDemo();
 
 canvas.present3D();
}

function webGLColorkey(css)
{
 css = String(css).toLowerCase();
 
 switch(css)
 {
 case '': 
 case 'none': //texture (black) knockout
 
  quat4.set([0,0,0,-2],canvas.colorKey); break;
  
 case 'transparent': //normal (ideal) knockout
 
  quat4.set([0,0,0,0],canvas.colorKey); break;
 
 default: //custom highlight problem areas
 
  canvas.cssColor(css,canvas.colorKey);  
 }
 
 canvas.present3D();
}

function webGLBackface(css)
{
 css = String(css).toLowerCase();
 
 switch(css)
 {
 case '': 
 case 'none': 
 case 'transparent': //disable
 
  quat4.set([0,0,0,0],canvas.backFace); break;
 
 default: //opaque webcolor 
 
  canvas.cssColor(css,canvas.backFace);
 }
 
 canvas.present3D();
}

canvas.cssColor = function(css,rgba)
{   
  //TODO: load rgbcolor.js dynamically
 
 var rgb = new RGBColor(css);

 if(rgb.ok) 
 {
  quat4.set([rgb.r/255,rgb.g/255,rgb.b/255,1],rgba);
 }
}

function webGLClean()
{
 //This should purge unnecessary memory
  
 if(canvas.loading)
 { 
  return canvas.cleaning = 
   setTimeout("webGLClean()",1000);
 }
 
 canvas.cleaning = clearTimeout(canvas.cleaning);
  
 canvas.cleanup();
}

/////3D PREPROCESSING FUNCTIONS//////

canvas.compose3D = function()
{  
 var x = canvas.composite++;
 
 var content = [];  
 for(var i in canvas.inputs) 
 if(!canvas.missing(canvas.inputs[i],canvas.min))
 {
  if(canvas.inputs[i].display)
  {    
   content.push(canvas.inputs[i].content);
  
   canvas.inputs[i].composite = x;
  }
 }
 else 
 {
  return canvas.loading = 
   setTimeout("canvas.compose3D()",1000);
 }  
 
 canvas.loading = clearTimeout(canvas.loading);
 
 if(!content.length) return;
  
 canvas.removeTracks();  
  
 //include reference graphics
 for(var i=0,ref;i<content.length;i++)
 {
  if(content[i].graphics)
  for(var j=0;j<content[i].graphics.length;j++)
  {
   if(ref=content[i].graphics[j].ref)
   if(canvas.inputs[ref].composite!=x)
   {
    content.push(canvas.inputs[ref].content);
     
    canvas.inputs[ref].composite = x;       
   }   
  }
 }
 
 canvas.prepare();
 
 canvas.clearance = false;
  
 var a=0,b=0,c=0;
 for(var i=0;i<content.length;i++)
 {      
  //We use slots to avoid duplication
  //Duplicate (unfilled) slots will result if
  //external resources are included more than once.   
  
  if(content[i].materials) //canvas.passes[]
  for(var j=0;j<content[i].materials.length;j++)
  a = canvas.slot3D(a,content[i],'materials',j);
    
  if(content[i].arrays) //canvas.buffers[] 
  for(var j=0;j<content[i].arrays.length;j++)
  b = canvas.slot3D(b,content[i],'arrays',j);    
  //counting textures as buffers too (less cleanup)
  if(content[i].images)
  for(var j=0;j<content[i].images.length;j++)
  b = canvas.slot3D(b,content[i],'images',j);        
  //the remaining slots are "linked" to the arrays  
  if(content[i].attributes)
  for(var j=0;j<content[i].attributes.length;j++)
  canvas.slot3D(0,content[i],'attributes',j,"array");
  //ditto       
  if(content[i].elements)
  for(var j=0;j<content[i].elements.length;j++)
  canvas.slot3D(0,content[i],'elements',j,"array");
    
  if(content[i].keygroups) //canvas.keys[]
  for(var j=0;j<content[i].keygroups.length;j++)
  c = canvas.slot3D(c,content[i],'keygroups',j);       
 }
  
 var n = 0; //passes 
 for(var i=0;i<=n;i++)
 {
  canvas.passes[i] = [];   
  for(var j=0;j<content.length;j++)
  //exclude any indirect references
  if(content[j].request.display&&content[j].graphics)
  {   
   for(var k=0;k<content[j].graphics.length;k++)
   n = canvas.pass3D(i,content[j],content[j].graphics[k],n);
  }  
 }
      
 var n=1; //tracks
 canvas.tracks.length = 1;
 canvas.tracks[0] = { number:'1', rot:true, key:[],pos:0 };
 for(var i=0;i<content.length;i++)
 //exclude any indirect references
 if(content[i].request.display&&content[i].tracks)
 {   
  for(var j=0;j<content[i].tracks.length;j++)
  {
   n = canvas.track3D(n,content[i],j);
  }
 }  
  
 //canvas.keys...
 n = canvas.keys.length;
 for(var i=0,key;i<n;i++)
 {
  var frames = canvas.keys[i].frames;
  for(var j=0,jN=frames.length;j<jN;j++)
  {
   //any left overs from past composites?
   delete frames[j]._forms; 
  }  
 }
 for(var i=0,key;i<n;i++)
 {
  if(key=canvas.keys[i],key.forms)
  {   
   var frames = key.frames;   
   for(var j=0,jN=frames.length;j<jN;j++)
   {
    var t = frames[j]._forms || [];
        
    frames[j]._forms = t.concat(key.forms); 
   }   
   delete key.forms;
  }
 }  
 for(var i=0,n=canvas.tracks.length;i<n;i++)
 {
  var key = canvas.tracks[i].key;  
  for(var j=0,jN=key.length;j<jN;j++) if(!key[j].forms)
  {  
   var frames = key[j].frames, forms = key[j].forms = [];
   
   for(var k=0,kN=frames.length;k<kN;k++)
   {
    var frame = frames[k], t = frame.timerange;    
    var formsk = forms[k] = frame._forms || [];
    
    var g = frame.graphics, f = 
    {
     min:t[0],max:t[1],run:t[1]-t[0]
    };    
    if(g) for(var l=0,ll,lN=formsk.length;l<lN;l++)
    {      
     var src = formsk[l].source;     
     for(var ll=l-1;ll>=0&&!f.forms[l];ll--)
     if(formsk[ll].source===src) f.forms[l] = f.forms[ll];
     if(ll>=0) continue;      
          
     if(!f.forms) f.forms = []; f.forms[l] = [];
     
     for(var ll=0;ll<g.length;ll++)
     f.forms[l][ll] = canvas.form3D(src,g[ll]);
    }    
     
    frames[k] = f;
   }  
  }
 }
 //_forms: deleting would require a 3rd pass
  
 //(since the keys are in tracks)
 //it's ok to toss these for now
 canvas.keys.length = 0; 

 //hack: important for memory management
 for(var i=0,iN=canvas.forms.length;i<iN;i++)
 {
  delete canvas.forms[i].source;
 } 
  
 //unfinished business 
 canvas.composing = false;
 for(var i=0;i<content.length;i++) 
 if(canvas.missing(content[i].request))
 {
  canvas.composing = true; break; 
 }
 
 canvas.addTracks();
 
 canvas.clear3D();
 canvas.view3D();
 canvas.crop3D(); 
 canvas.play3D(); 
}

canvas.slot3D = function(n,context,key,index,link)
{ 
 var item = context[key][index];  
 var subcontext = canvas.follow(item);
 
 var subitems; 
 if(!subcontext)
 {
  if(link) //quasi hackish link mode
  {
   var found = 
   canvas.find(context,n?n:link+'s',item[link]);
   
   item.slot3D = found?found.slot3D:null;
  }
  else item.slot3D = n++; subitems = item[key];
  
  switch(key) //a few economical hacks
  {    
  case 'images': 
  
   if(item.rgb&&item.url===undefined)
   {
    item.url = canvas.rgbData(item.rgb); item.inf = true;
   } 
   
   var texture = canvas.readyTexture(item.url);
  
   canvas.buffers[item.slot3D] = texture; break;
   
  case 'arrays': canvas.buffer3D(item); break;
  
  case 'keygroups': var frames =
   
   canvas.collect(context,'keyframes',item.keyframes,[]);
   
   canvas.keys[item.slot3D] = {group:item,frames:frames};
  } 
 }
 else subitems = subcontext[key];
 
 if(subitems) for(var i=0;i<subitems.length;i++)
 {  
  n = canvas.slot3D(n,subcontext,key,i,link);
 }
 
 return n; 
}

canvas.buffer3D = function(array)
{ 
 var buffer = 
 canvas.buffers[array.slot3D] = canvas.gl.createBuffer();
 
 if(!buffer) return;
 
 if(!array.index)
 {
   //save everyone some memory (see delete array.data below) 
   if(!array.dataGL) array.dataGL = new Float32Array(array.data);
   
   canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER,buffer);
   canvas.gl.bufferData(canvas.gl.ARRAY_BUFFER,array.dataGL,canvas.gl.STATIC_DRAW);
   buffer.type = canvas.gl.FLOAT; buffer.typesize = 4;
 }
 else 
 {
  canvas.gl.bindBuffer(canvas.gl.ELEMENT_ARRAY_BUFFER,buffer);
  
  array.sizeGL = array.sizeGL || Math.max.apply(null,array.data);
   
  if(array.sizeGL>65535)
  {
   if(!array.dataGL) array.dataGL = new Uint32Array(array.data);
   canvas.gl.bufferData(canvas.gl.ELEMENT_ARRAY_BUFFER,array.dataGL,canvas.gl.STATIC_DRAW);   
   buffer.type = canvas.gl.UNSIGNED_INT; buffer.typesize = 4;
  }
  else 
  {
   if(!array.dataGL) array.dataGL = new Uint16Array(array.data);
   canvas.gl.bufferData(canvas.gl.ELEMENT_ARRAY_BUFFER,array.dataGL,canvas.gl.STATIC_DRAW);
   buffer.type = canvas.gl.UNSIGNED_SHORT; buffer.typesize = 2;
  }
 }
 
 delete array.data;
}

canvas.form3D = function(context,graphic,frame)
{ 
 var t = frame?false:true; //assume template 
 
 var form = {}; if(!frame) frame = form;
 
 if(t||graphic.primitives&&graphic.primitives.length)
 {
  if(!t||graphic.attributes) frame.attr = form.attr = 
  canvas.collect(context,'attributes',graphic.attributes);
  
  if(!t||graphic.primitives) frame.elem = form.elem = 
  canvas.collect(context,'elements',graphic.primitives);  
  
  //template hack
  if(t&&form.attr) //An inline object
  for(var i=0,a;i<form.attr.length;i++)
  if(a=form.attr[i],a.slot3D===undefined)
  {
   var b = canvas.find(context,'arrays',a.array);
   
   if(b&&b.slot3D>=0&&b.slot3D<canvas.buffers.length)
   {
    a.slot3D = b.slot3D; //could do a boundary check 
   }
   else a.array = null;
  }  
    
  if(form.elem) 
  for(var i=0;i<form.elem.length;i++)
  {
   form.elem[i].modeGL = 
   canvas.constantGL(form.elem[i].mode,"TRIANGLE");    
  }
     
  if(!t) //not supporting these for templates...
   
  if(graphic.blend)
  {
   form.opacity = graphic.opacity||1.0;
   
   form.blendGL = 
   [
    canvas.constantGL(graphic.blend[0],"ONE"),
    canvas.constantGL(graphic.blend[1],"ZERO"),
   ]; 
  }
  else if(graphic.sides)
  {
   //canvas.reverse3D()
   if(graphic.sides[0]==null)
   {
    form.back = canvas.gl.FRONT;
   }
   else if(graphic.sides[1]==null)
   {
    form.back = canvas.gl.BACK;
   }
  }
 }
 
 if(graphic.state)
 {
  if(graphic.state===1)
  {
   graphic.stateGL = canvas.identity;
  }
  else if(!graphic.stateGL)
  {
   graphic.stateGL = new Float32Array(graphic.state);
   
   graphic.state = true; //save everyone some memory
  }
  
  form.state = frame.state = graphic.stateGL;
 }
 
 if(!t) form.n = 0; 
 return form;
}

canvas.pass3D = function(n,context,graphic,maxpass)
{  
 var subgraphics = 0;
 var subcontext = canvas.follow(graphic);
 
 if(!subcontext)
 {
  var pass = graphic.pass||0;
  
  if(pass>maxpass) maxpass = pass;
  
  if(pass===n)
  {       
   var frame = {}; //animation 1/2 of form      
   var form = canvas.form3D(context,graphic,frame);
   
   canvas.forms.push(form);
       
   if(graphic.clearance) 
   {
    form.clearance = graphic.clearance;    
    canvas.clearance = true;
   }   
   
   if(graphic.keygroups)
   {
    form.source = context; //hack: to be deleted by compose3D  
    
    var groups = canvas.collect(context,'keygroups',graphic.keygroups,[]);
    
    for(var i=0;i<groups.length;i++)
    {
     var key = canvas.keys[groups[i].slot3D];
     
     (key.forms=key.forms||[]).push(form);
    }    
   }
   
   if(form.elem)
   for(var app,$=0;$<2;$++)
   if(graphic.sides&&graphic.sides[$]!=null)
   {       
    var material = canvas
    .find(context,'materials',graphic.sides[$]);
            
    if(!material) continue; //paranoia  
   
    //if(form.app) //Assuming forms are single sided!
    /*SOMETHING will have to be done about app/fid*/ 
    
    if(app=canvas.passes[n][material.slot3D])
    {
     form.app = app; form.fid = app.forms.length;
          
     app.forms.push(form); app.frames.push(frame); 
     
     continue;     
    }    
    //else: application initialization...
    
    var D;
    var source = canvas.find.context; //hack
        
    if(material.semantics)
    {
     var vs = canvas.find(source,'semantics',material.semantics[0]);
     
     for(var i in vs) if(vs[i].glsl)
     {
      if(!D) D = ''; D+='#define '+i.toUpperCase()+' '+vs[i].glsl+'\n';
     }     
    }  
    
    var prog = canvas.needDefault(D);
    
    if(D&&prog) //shadow/reverse3D
    {
      prog.shadow = canvas.needShadow(D);
      prog.reverse = canvas.needSolid(D);
    }
     
    var tex = canvas.collect(source,'images',material.texture,[]);
     
    form.fid = 0; //first frame/form id
    form.app = app = canvas.passes[n][material.slot3D] =     
    {     
     material:material,program:prog,texture:tex,
     
     forms:[form],frames:[frame]
    };
   }
  }
  
  subgraphics = graphic.graphics;  
  subcontext = context;  
 }
 else subgraphics = subcontext.graphics;
 
 if(subgraphics) for(var i=0;i<subgraphics.length;i++)
 {  
  maxpass = canvas.pass3D(n,subcontext,subgraphics[i],maxpass);
 }
 
 return maxpass;
}

canvas.track3D = function(n,context,index,outline)
{ 
 if(!outline) outline = '';
 
 var item = context.tracks[index];  
 var subcontext = canvas.follow(item);
 
 var subitems; 
 if(!subcontext)
 { 
  var track = canvas.tracks[n] = 
  {
   item:item,number:outline+(n+1),key:[],pos:n
  };
  
  if(item.key) 
  {
   var groups = 
   canvas.collect(context,'keygroups',item.key,[]);
   
   for(var i=0;i<groups.length;i++) if(groups[i])
   {
    track.key.push(canvas.keys[groups[i].slot3D]);
   }
  }
  
  subitems = item.tracks; n++;   
 }
 else subitems = subcontext.tracks;
 
 if(subitems) for(var i=0;i<subitems.length;i++)
 {  
  n = canvas.track3D(n,subcontext,i,outline+(index+1)+'.');
 }
 
 return n; 
}

/////3D RENDERING FUNCTIONS///////////

canvas.present3D = function()
{ 
 if(canvas.scale<=0)
 {
  if(canvas.scale==0&&--canvas.scale)
  canvas.gl.clear(canvas.gl.COLOR_BUFFER_BIT);  
  return;
 }
 
 if(canvas.loading)
 { 
  return canvas.waiting = 
   setTimeout("canvas.present3D()",1000);
 }
 
 if(!canvas.passes.length) return;
 
 canvas.waiting = clearTimeout(canvas.waiting);
  
 if(canvas.element.width!=canvas.element.clientWidth*canvas.x
 ||canvas.element.height!=canvas.element.clientHeight*canvas.x)
 {
  canvas.element.height = canvas.element.clientHeight*canvas.x;
  canvas.element.width = canvas.element.clientWidth*canvas.x;
  
  canvas.view3D(); canvas.crop3D();
 }  
 
 //NOTE: It would be nice if the canvas IS the viewport at some point
 //As is the canvas encompasses the window even if the viewport is miniscule
 //Reminder: this would probably require repositioning the canvas element.
 
 var vp = canvas.viewport; 
 canvas.gl.viewport(vp[0],vp[1],vp[2],vp[3]); 
 canvas.gl.clear(canvas.gl.COLOR_BUFFER_BIT|canvas.gl.DEPTH_BUFFER_BIT);
    
 canvas.shadow3D(); //WARNING: opaque/depthwrite

 if(canvas.debug) canvas.debug3D();
 
 canvas.program = null;
 for(var i=0;i<canvas.passes.length;i++)
 for(var j=0;j<canvas.passes[i].length;j++)
 {
  var app = canvas.passes[i][j];
  
   //fyi: a missing app just means 
  //a material is not used this pass.  
  if(!app) continue; 
 
  var prog = app.program;
  canvas.gl.useProgram(prog);
  
  for(var k=0;k<app.texture.length;k++)
  {
   var tex = app.texture[k];
   var img = canvas.buffers[tex.slot3D];
   
   if(img&&img.tex2D) 
   {
    canvas.gl.activeTexture(canvas.gl.TEXTURE0+k);
    canvas.gl.bindTexture(canvas.gl.TEXTURE_2D,img.tex2D);
    
    //hack: setup texture matrix
    //TODO: support state/rotate/translate
    if(tex.scale&&k===0&&app.texture.length==1)
    {
     canvas.texMatrix[0] = tex.scale[0];
     canvas.texMatrix[4] = tex.scale[1];     
    }
   }
  }
  
  var rgba = 
  canvas.rgb(app.material.diffuse,[1,1,1,1]);  
  quat4.set(rgba,canvas.diffuseCol);  
  rgba = canvas.rgb(app.material.emissive,[0,0,0,0]);  
  quat4.set(rgba,canvas.emissiveCol);
        
  canvas.setProgramUniforms(prog);
  
  canvas.apply3D(app,function(f)
  {   
   if(f.blendGL)
   {
    if(prog.opacityUniform) 
    canvas.gl.uniform1f(prog.opacityUniform,f.opacity);
    
    canvas.gl.depthMask(false);
    canvas.gl.enable(canvas.gl.BLEND);    
    canvas.gl.blendFunc(f.blendGL[0],f.blendGL[1]); 
   }
   else 
   {
    //canvas.gl.depthFunc(canvas.gl.LESS);
    canvas.gl.disable(canvas.gl.BLEND);
    canvas.gl.depthMask(true);
   }
  });
   
  canvas.texMatrix[0] = canvas.texMatrix[4] = 1;  
 }
  
 canvas.gl.disable(canvas.gl.BLEND);
 canvas.gl.depthMask(true); 
 
 if(canvas.backFace[3]!=0)
 canvas.reverse3D(); 
}

canvas.shadow3D = function()
{
 if(!canvas.shadow) return;
 
 var prog = canvas.needShadow();
     
 canvas.gl.depthFunc(canvas.gl.LESS);
 canvas.gl.disable(canvas.gl.CULL_FACE);
  
 var flr = [0,1,0,0];  
 if(canvas.clearance) flr[3] = -canvas.clearance[1][0];
  
 var inv = mat4.inverse(canvas.mvMatrix,[]); 
 var eye = mat4.multiplyVec3(inv,[0,0,0]);
 
 var sign = vec3.dot(flr,eye)+flr[3]>0?1:-1;
 
 //2: reverse3D has 1 now
 //0.01: seems to help close to znear
 canvas.gl.polygonOffset(0.01*sign,sign*2);
 canvas.gl.enable(canvas.gl.POLYGON_OFFSET_FILL); 
 
 var lit = vec3.set(canvas.light,[]); lit[3] = 1;
 
 var dot = vec3.dot(flr,lit); dot+=flr[3]*1;
 
 //assuming horizontal
 //http://www.opengl.org/resources/features/StencilTalk/tsld021.htm
 var mat = mat4.create( 
 [
  dot,   -lit[0]*flr[1],0  ,   -lit[0]*flr[3], 
  0  ,dot-lit[1]*flr[1],0  ,   -lit[1]*flr[3],
  0  ,   -lit[2]*flr[1],dot,   -lit[2]*flr[3],
  0  ,   -lit[3]*flr[1],0  ,dot-lit[3]*flr[3] 
 ]); 
 
 mat4.transpose(mat);
 
 var mvp = mat4.create();
    
 //TODO: what about canvas.mvpMatrix?
 mat4.multiply(canvas.mMatrix,mat,mvp);     
 mat4.multiply(canvas.vMatrix,mvp,mvp); 
 mat4.multiply(canvas.pMatrix,mvp,mvp);
   
 //should not be baked in.
 var c = 8/255, f = 11/255;
 
 var using; 
 for(var i=0;i<canvas.passes.length;i++)
 for(var j=0;j<canvas.passes[i].length;j++)
 {
  var app = canvas.passes[i][j]; if(!app) continue;
   
  if(using!=(canvas.program=app.program.shadow||prog))
  {
   canvas.gl.useProgram(using=canvas.program);   
    
   canvas.gl.uniformMatrix4fv(using.shadowMatUniform,false,mat);
   canvas.gl.uniformMatrix4fv(using.mvpMatrixUniform,false,mvp);
     
   canvas.gl.uniform3f(using.shadowColUniform,0,c,c);  
   canvas.gl.uniform3f(using.fogColUniform,0,f,f); 
   canvas.gl.uniform1f(using.fogRadiusUniform,canvas.radius);
   
   canvas.gl.uniform3fv(using.rotateUniform,canvas.rotate);
   
   canvas.gl.uniformMatrix4fv(using.stateUniform,false,canvas.identity);
  }
    
  canvas.apply3D(app);
 }
 
 canvas.gl.depthFunc(canvas.gl.LEQUAL);
 canvas.gl.enable(canvas.gl.CULL_FACE);
 canvas.gl.disable(canvas.gl.POLYGON_OFFSET_FILL);
 canvas.gl.polygonOffset(0,0);
}

canvas.reverse3D = function()
{
 var prog = canvas.needSolid();
          
 canvas.gl.depthFunc(canvas.gl.LESS);
 
 //canvas.gl.polygonOffset(0,-1);
 ////canvas.gl.enable(canvas.gl.POLYGON_OFFSET_FILL); 
 
 var using;
 for(var i=0;i<canvas.passes.length;i++)
 for(var j=0;j<canvas.passes[i].length;j++)
 {
  var app = canvas.passes[i][j]; if(!app) continue;
  
  //Google Chrome: "WARNING: there is no texture bound to the unit 0"
  //canvas.gl.bindTexture(canvas.gl.TEXTURE_2D,null);
   
  if(using!=(canvas.program=app.program.reverse||prog))
  {
   canvas.gl.useProgram(using=canvas.program);
      
   if(prog.mvpMatrixUniform) //hack: assuming mvpMatrix has been setup  
   canvas.gl.uniformMatrix4fv(using.mvpMatrixUniform,false,canvas.mvpMatrix);
   if(prog.solidColUniform) 
   canvas.gl.uniform4fv(using.solidColUniform,canvas.backFace);
   
   canvas.gl.uniform3fv(using.rotateUniform,canvas.rotate);
   
   canvas.gl.uniformMatrix4fv(using.stateUniform,false,canvas.identity);
  }
     
  var tex = app.texture.length?app.texture[0]:0;
    
  if(tex&&canvas.colorKey[3]==0.0)
  if(tex.alpha===undefined||tex.alpha===true)
  {   
   var img = canvas.buffers[tex.slot3D];
   
   if(img&&img.tex2D) 
   {
    canvas.gl.activeTexture(canvas.gl.TEXTURE0);
    canvas.gl.bindTexture(canvas.gl.TEXTURE_2D,img.tex2D);
            
    //hack: see present3D for more comments    
    if(tex.scale&&app.texture.length==1&&using.texMatrixUniform)
    {
     canvas.texMatrix[0] = tex.scale[0]; canvas.texMatrix[4] = tex.scale[1];     
     canvas.gl.uniformMatrix3fv(using.texMatrixUniform,false,canvas.texMatrix);     
    }
   }
  }
  
  canvas.apply3D(app,function(f)
  {    
   if(f.back===undefined) return true;  
   
   canvas.gl.cullFace(f.back);
   
   if(f.back===canvas.gl.FRONT)
   {
     canvas.gl.frontFace(canvas.gl.CCW);
   }
   else canvas.gl.frontFace(canvas.gl.CW);  
  });
  
  //hack: this needs a better system
  if(canvas.texMatrix[0]*canvas.texMatrix[4]!=1)
  {
   canvas.texMatrix[0] = canvas.texMatrix[4] = 1;
   canvas.gl.uniformMatrix3fv(using.texMatrixUniform,false,canvas.texMatrix);
  }
 }
 
 canvas.gl.frontFace(canvas.gl.CCW);
 canvas.gl.cullFace(canvas.gl.BACK);
 canvas.gl.depthFunc(canvas.gl.LEQUAL);
 //canvas.gl.disable(canvas.gl.POLYGON_OFFSET_FILL);
 //canvas.gl.polygonOffset(0,0);
}

canvas.apply3D = function(app,g)
{
 var n = canvas.n;
 
 var forms = app.forms; 
 var frames = n?app.frames:forms;
 
 var prog = canvas.program||app.program;  
 var at = prog.attributes;
  
 var f, t, a, p, e, state;
 var lM, lN, kN = forms.length; 
 for(var k=0;k<kN;k++) if(f=frames[k])
 { 
  if(f.n!=n||g&&g(forms[k],f)) continue;
 
  if(t!==(f.t||0)) 
  canvas.gl.uniform1f(prog.tweenUniform,t=f.t||0);
  
  if(state!=f.state)
  canvas.gl.uniformMatrix4fv(prog.stateUniform,false,state=f.state);
    
  lM = f.attr.length; lN = f.elem.length;
  
  for(var l=0;l<lM;l++) if(a=f.attr[l])
  {       
   if((p=at[a.semantic])===undefined) continue;

   var abuf = canvas.buffers[a.slot3D];   
   var psize = a.size||3; //most common 
   var offset = (a.start||0)*abuf.typesize;        
   var stride = (a.stride||0)*abuf.typesize;
   canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER,abuf);
   canvas.gl.vertexAttribPointer(p,psize,canvas.gl.FLOAT,false,stride,offset);
   canvas.gl.enableVertexAttribArray(p);    
  }     
  for(var l=0;l<lN;l++) if(e=f.elem[l])
  {   
   var ebuf = canvas.buffers[e.slot3D];
   var eoff = (e.start||0)*ebuf.typesize;        
   
   canvas.gl.bindBuffer(canvas.gl.ELEMENT_ARRAY_BUFFER,ebuf);
   
   canvas.gl.drawElements
   (e.modeGL,e.count,ebuf.type,eoff);
  }
 }
}
   
canvas.debug3D = function()
{
 var prog = canvas.needDebug();
 
 if(prog) canvas.gl.useProgram(prog); else return;
 
 if(typeof canvas.clearance=='object')
 {
  if(!canvas.debug3D.cube)
  {   
   var unit = 
   [-1,+1,+1, +1,+1,+1,
     -1,+1,-1, +1,+1,-1, 
   
    -1,-1,+1, +1,-1,+1,
     -1,-1,-1, +1,-1,-1,     
   ];
   
   var cube = //A laser light show
   [
    0,1,2,3,0, 4,5,6,7,4, //top/bottom
    0,2,4,6,0, 1,3,5,7,1, //left/right
    2,6,3,7,2, 0,4,1,5,0, //front/back
    7,3,4,6,1,5,3        //criss/cross 
    
   ];
   
   if(canvas.debug3D.unit=canvas.gl.createBuffer())
   if(canvas.debug3D.cube=canvas.gl.createBuffer())
   {    
    canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER,canvas.debug3D.unit);
    canvas.gl.bufferData(canvas.gl.ARRAY_BUFFER,new Float32Array(unit),canvas.gl.STATIC_DRAW);
    
    canvas.gl.bindBuffer(canvas.gl.ELEMENT_ARRAY_BUFFER,canvas.debug3D.cube);
    canvas.gl.bufferData(canvas.gl.ELEMENT_ARRAY_BUFFER,new Int16Array(cube),canvas.gl.STATIC_DRAW);
   }
   else return;
  }
   
  canvas.gl.bindBuffer(canvas.gl.ARRAY_BUFFER,canvas.debug3D.unit);
  canvas.gl.vertexAttribPointer(prog.position,3,canvas.gl.FLOAT,false,0,0);
  canvas.gl.enableVertexAttribArray(prog.position);
  
  var tmp = mat4.multiply(canvas.mvMatrix,canvas.bbox,[]);
  
  mat4.multiply(canvas.pMatrix,tmp,canvas.mvpMatrix);
  canvas.gl.uniformMatrix4fv(prog.mvpMatrixUniform,false,canvas.mvpMatrix);
 
  canvas.gl.disableVertexAttribArray(prog.color); //initDemo()
  canvas.gl.vertexAttrib4f(prog.color,1,0,0,1);
  
  canvas.gl.bindBuffer(canvas.gl.ELEMENT_ARRAY_BUFFER,canvas.debug3D.cube);  
  canvas.gl.drawElements(canvas.gl.LINE_STRIP,37,canvas.gl.UNSIGNED_SHORT,0);
  
  canvas.gl.disableVertexAttribArray(prog.position);             
 }
}

//// MISCELANEOUS 3D FUNCTIONS /////

canvas.clear3D = function()
{ 
 if(!canvas.clearance) return;
 
 var min = -Number.MAX_VALUE, max = Number.MAX_VALUE;
 
 canvas.clearance = [[max,min],[max,min],[max,min]];
 
 var a = canvas.clearance, b;
 for(var i=0;i<canvas.forms.length;i++)
 {   
  if(b=canvas.forms[i].clearance)
  {
   a[0][0] = Math.min(a[0][0],b[0][0]);
   a[0][1] = Math.max(a[0][1],b[0][1]);
   a[1][0] = Math.min(a[1][0],b[1][0]);
   a[1][1] = Math.max(a[1][1],b[1][1]);
   a[2][0] = Math.min(a[2][0],b[2][0]);
   a[2][1] = Math.max(a[2][1],b[2][1]);
  }
 }
  
 var vmin = [a[0][0],a[1][0],a[2][0]]; 
 var vmax = [a[0][1],a[1][1],a[2][1]];
   
 canvas.pivot = vec3.lerp(vmax,vmin,0.5,[]);
  
 var difference = vec3.subtract(vmax,canvas.pivot,[]);
    
 canvas.radius = vec3.length(difference);
  
 canvas.bbox = 
 [
  difference[0],0,0,0,
  0,difference[1],0,0,
  0,0,difference[2],0,
  canvas.pivot[0],
  canvas.pivot[1],
  canvas.pivot[2],1  
 ]; 
}

canvas.view3D = function()
{
 var d = 7, p;  
 if(canvas.clearance)
 {
  d = canvas.radius; p = vec3.set(canvas.pivot,[]); 
 }
 else p = [0,0,0];
  
 var z = canvas.convention[0]>=0?1:-1; 
 var v = mat4.identity(canvas.vMatrix); 
 
 //Establish view distance
 mat4.translate(v,[0,0,z*d*(1+znear)]);
    
 //the frontal overhead light for the shader
 quat4.set([0,d*1.1,-z*d*0.5,1/(d*0.5)],canvas.light);  
 
 if(canvas.clearance) //scale view as necessary
 {  
  //point on the bounding sphere of the model
  var r = vec3.scale(vec3.normalize([1,1,-z,1]),d);
  
  //Note: there is probably a more elegant way to 
  //go about this than projecting into screen space... 
    
  var vp = //projection part 
  mat4.perspective(45,canvas.element.clientWidth
  /canvas.element.clientHeight,znear,zfar,[]);
  
  mat4.multiply(vp,v); //view/projection matrix  
  mat4.multiplyVec4(vp,r); //project r to screen
  
  var scale = 1/((r[1]/r[3]+1)*0.5); //1/divide
  
  scale*=0.8; //make room for a little border/shadow
  
  mat4.scale(v,[scale,scale,scale]);
 }

 if(canvas.d[1]) //1D roll
 {
 //would prefer this be performed by crop3D
  
  var r = Number(canvas.d[1][0])/100-0.5;
     
  mat4.rotateZ(v,Math.PI*2*r);
 }
 
 //bring view to model level
 mat4.translate(v,vec3.negate(p));
 
 var m = mat4.identity(canvas.mMatrix); //model matrix
    
 //get 3d rotation numbers (-0.5,+0.5) 
 var x = Number(canvas.d[3][0])/100-0.5;
 var y = Number(canvas.d[3][1])/100-0.5;
    
 //get inverse of left/right rotation
 var inv = mat4.rotateY(m,Math.PI*2*x,[]);
 
 //get stationary up/down rotation axis 
 var h = mat4.multiplyVec3(inv,[1,0,0]); 
 
 //move pivot to center of view
 mat4.translate(m,vec3.negate(p));
  
 //apply left/right rotation
 mat4.rotateY(m,Math.PI*2*-x);
 
 //apply up/down rotation
 mat4.rotate(m,-Math.PI*y,h);
 
 //undo pivot to center of view   
 mat4.translate(m,vec3.negate(p));
 
 /////////////////////////////////////////////
   
 //merge model matrix and view matrix
 mat4.multiply(v,m,canvas.mvMatrix);
}

canvas.crop3D = function()
{
 //A lot of this is because a viewport
 //must be restricted to the window body.
 
 canvas.scale = 1; //assuming scaled to screen
  
 if(canvas.d[1]) //1D roll/scale
 { 
  canvas.scale = Number(canvas.d[1][1])/50;
 }
 if(canvas.d[2])  
 {  
  canvas.pan[0] = -(Number(canvas.d[2][0])/100-0.5);
  canvas.pan[1] = +(Number(canvas.d[2][1])/100-0.5); 
 }
 
 var n = 0.1*canvas.radius; //znear;
 var f = 3*canvas.radius; //zfar;
  
 var w = canvas.element.width;
 var h = canvas.element.height;  
 var x = canvas.pan[0]*h*canvas.scale; 
 var y = canvas.pan[1]*h*canvas.scale; 
   
 var crop = canvas.scale*h;
 
 var widen = 1.2; //a little room for shadow
     
 //hard coding right-center alignment
 //TODO: maybe an offscreen indicator?
 
 var l = x+(w-crop*widen), t = y+0.5*(h-crop);
 
 if(l>=w||t>=h) return false; //offscreen!
 
 var r = l+crop*widen, b = t+crop;
 
 if(r<=0||b<=0) return false; //offscreen!
 
 var c = [(l+(r-l)/2),t+(b-t)/2]; //center
  
 //Per swoop applied down below 
 {  
  l+=canvas.scale*(l-c[0]);
  r+=canvas.scale*(r-c[0]);
  t+=canvas.scale*(t-c[1]);
  b+=canvas.scale*(b-c[1]); 
 }
 
 l = Math.max(l,0); r = Math.min(r,w);
 t = Math.max(t,0); b = Math.min(b,h);
  
 canvas.viewport = [l,t,r-l,b-t];
 
 l = (l-c[0])/crop*n;  
 r = (r-c[0])/crop*n;
 t = (t-c[1])/crop*n;
 b = (b-c[1])/crop*n; 
  
 mat4.frustum(l,r,t,b,n,f,canvas.pMatrix);
 
 var s = canvas.scale;
 //scaling here is not necessary but it 
 //results in a very cool swooping effect! 
 mat4.scale(canvas.pMatrix,[s,s,1]);
 
 return true;
}

canvas.play3D = function(overlap)
{
 if(!canvas.display) return;
 
 var track = canvas.tracks[canvas.t]; 
 
 if(!track||!track.key.length&&!track.rot) 
 {
 //changing to play list with fewer tracks?
 
  if(canvas.loop) //keep the track in use
  {
   canvas.n = 0; return canvas.present3D(); 
  }
  else canvas.changeTrack(0);
  
  track = canvas.tracks[0];
 }
 
 if(track.time===undefined)
 {
  canvas.readyTrack(track); 
 
  if(overlap===undefined) //preset
  {
   track.time = track.run*((100-canvas.d[4][0])/100);   
  }
  else if(canvas.speed<0) //backward
  {
   track.time = track.run; 
  }
 }
  
 if(overlap!==undefined) 
 {
  if(canvas.speed<0) overlap-=0.0001;
  
  track.time = (track.run+overlap)%track.run;
 }
 
 var still = !canvas.play||track.still;
   
 if(!still)
 {
  var now = +Date.now()/1000; //+new Date
   
  if(track.pause===true)
  {
   track.now = now; track.pause = false;
  }
 
  //give up on real-time below 30fps
  var delta = Math.min(now-track.now,0.03333);
   
  track.time+=delta*canvas.speed; track.now = now;
  
  if(canvas.speed>0)
  {
   if(track.time>track.run)
   {  
    if(!canvas.loop)
    {
     canvas.changeTrack(canvas.t+1);
     
     //hack: loops better without 360 
     if(!canvas.t) canvas.changeTrack(1);
    }
    
    var overlap = track.time-track.run; track.time = 0;
    
    return canvas.play3D(overlap);     
   }
  }
  else if(track.time<0)
  {  
   if(!canvas.loop)
   {
    canvas.changeTrack(canvas.t-1);
    
    //hack: loops better without 360 
     if(!canvas.t) canvas.changeTrack(-1);
   }
   
   var overlap = track.time; track.time = 0;
   
   return canvas.play3D(overlap);     
  }
 }
 else track.pause = !track.still;
  
 var t = track.time/track.run;
 
 if(track.rot)
 {
  var v = canvas.convention;    
  var p = Math.PI, r = t*p*2;  
  var c = Math.cos(v[0]*p-r);
  var s = Math.sin(v[1]*p-r);
  
  vec3.set([c,1,s],canvas.rotate); 
 } 

 canvas.frame3D(track.key,track.time); 
 
 canvas.present3D();
 
 if(canvas.playing=!still)
 {   
  canvas.d[4][0] = String(100-Math.round(t*100));
  if(canvas.d[0]===4) canvas.js_xy('x',canvas.d[4][0]);
  
  canvas.refresh3D(); //will loop back to play3D
 }
 else if(!canvas.loop&&canvas.play) 
 {
  //still: arbitrary 5 second slideshow
  
  setTimeout(canvas.play3D,5000);
 }
}

canvas.frame3D = function(keys,time)
{
 var iN = keys.length;
  
 if(!iN) return canvas.n = 0;
 
 canvas.n = ++canvas.frame; 
 
 for(var i=0;i<iN;i++)
 {
  var key = keys[i];
  
  var f = key.frames;
  var t = time/key.speed+key.min;
  
  //Reminder (about rounding error)
  //Assuming inter key overlap is ok. 
  
  if(t<key.min) //out of bounds?
  {
   if(t<key.min-0.0001) continue;
   
   t = key.min; //assume rounding error
  }  
  if(t>key.max) //out of bounds? 
  {
   if(t>key.max+0.0001) continue;
   
   t = key.max; //assume rounding error
  }
    
  //perfect boundary frames will
  //just be hit twice (seems safe)
  for(var k=0,kN=f.length;k<kN;k++)
  {
   if(t>=f[k].min&&t<=f[k].max) //<
   {
    var tween = (t-f[k].min)/f[k].run; 
    
    var forms = keys[i].forms[k];   
    for(var l=0,lN=forms.length;l<lN;l++)
    {
     var form = forms[l];    
     var frame = form.app.frames[form.fid];
     var frame_t = f[k].forms[l][0];
     
     for(var x in frame_t) frame[x] = frame_t[x]; 
          
     frame.n = canvas.frame;     
     frame.t = tween;
    }  
   } 
  }
 }
}

canvas.render3D = function() //use refresh3D instead
{
 if(canvas.playing) canvas.play3D(); else canvas.present3D(); 
}

//see requestAnimationFrame.js 
if(window.requestAnimationFrame)
{  
 canvas.refresh3D = function()
 {
  //want to be sure this stuff is atomic
  window.cancelAnimationFrame(canvas.refreshing);
    
  canvas.refreshing = 
  window.requestAnimationFrame(canvas.render3D,canvas.element);  
 }; 
}
else canvas.refresh3D = canvas.throttle(canvas.render3D,15);

/////////SEARCH FUNCTIONS////////////

//dereference item by any kind of index
canvas.find = function(context,key,index)
{   
  canvas.find.context; //hack(1of2)
  
  var list = 0;
 
  if(!index) index = 0;
        
  switch(typeof index)
  {
  case "string": //name
  
   return canvas.lookup(context,key,index);
  
  case "number": list = [index]; break;
  
  case "object": 
  
   //assuming an inline object
   if(index.length===undefined) return index;
     
   //assuming a list of numbers
   list = index; break;
   
  default: return;
  }
  
  //lazy addressing options explained
  //
  //underflow: instead of 0.0 when going
  //through an included resource (follow) 
  //you can get away with just 0. Same deal
  //for 1.0.0 ... 1 works.
  //
  //overflow: Instead of 1.1 you can get 
  //away with just 2 if there is nothing
  //above 1 in the current context and 1
  //is an included resource.
  
  //context is tracked below just 
  //for canvas.find.context sake...
  
  var level = context, found;
   
  while(list.length) //>1
  {
   //if(!level[key]) return;
   //if(undefined==level[key]) return;
   if(!level[key].length) return;
   
   var m = list.shift();
   var n = level[key].length-1;
   
   while(m>n) //lazy overflow addressing
   {    
    level = context = canvas.follow(level[key][n]); 
    
    if(!level||!level[key]) return;      
    
    m-=n; n = level[key].length-1;
   }
   
   //always follow links if able
   found = canvas.follow(level=level[key][m]);
   
   if(found) level = context = found;
  }
  
  while(found) //lazy underflow addressing 
  {
   if(!level[key]) return; 
   
   found = canvas.follow(level=level[key][0]);
   
   if(found) level = context = found;
  }
  
  canvas.find.context = context; //hack(2of2)
  
  return level; 
}

//dereference item by a unique name index
canvas.lookup = function(context,key,name)
{
 //TODO: build namespaces on demand.
 //Names are expected to be unique.
 
 var found, lookin = context[key]; 
 for(var i=0;i<lookin.length&&!found;i++)
 {
  if(lookin[i].name===name) found = lookin[i];
 } 
 //and recursive search...
 for(var i=0;i<lookin.length&&!found;i++)
 {
  if(context=canvas.follow(lookin[i]))
  found = canvas.lookup(context,key,name);
 } 

 return found;
}

canvas.collect = function(context,key,indices,def)
{
 var out = [];
 
 if(!indices) indices = def;
 
 if(!indices) //defaults
 {  
  var allin = context[key];
  
  if(allin.length) 
  for(var i=0;i<allin.length;i++)
  {
   var found = canvas.find(context,key,i);
   
   if(found) out.push(found);
  }
 }
 else for(var i=0;i<indices.length;i++)
 {
  var found = canvas.find(context,key,indices[i]);
  
  if(found) out.push(found);
 } 
 
 return out;
}

//follow a link to its context
canvas.follow = function(item)
{
 if(item.inf) return; //circular url (eg. an image) 
 if(item.ref) return canvas.inputs[item.ref].content; 
 if(item.url) return canvas.requests[item.url].content;
}

canvas.missing = function(request,priority)
{ 
 if(!request) return false;
 
 if(request.missing) return true; 
 if(request.infinite) return false;
 
 var externals = request.content.externals;

 if(!priority) priority = Number.MAX_VALUE;
 
 if(externals) 
 for(var i=0;i<externals.length;i++)
 if(externals[i].priority<=priority)
 {
  for(var j in externals[i])
  {
   var missing = 
   canvas.missing(canvas.requests[externals[i][j]],priority);
   
   if(missing) return true;
  }
 }
 
 return false;
}

canvas.rgb = function(item,rgb)
{
 if(!item||!item.rgb) return rgb; //default 
 
 if(item.rgb[0]!==undefined) rgb[0] = item.rgb[0];
 if(item.rgb[1]!==undefined) rgb[1] = item.rgb[1];
 if(item.rgb[2]!==undefined) rgb[2] = item.rgb[2];
 if(item.rgb[3]!==undefined) rgb[3] = item.rgb[3];
 
 return rgb;
}

canvas.rgbData = function(rgb) //1x1 texture
{
 if(!canvas.rgbDataCanvas)
 {
  canvas.rgbDataCanvas = document.createElement('canvas');    
  canvas.rgbDataContext = canvas.rgbDataCanvas.getContext('2d');
    
  canvas.rgbDataCanvas.width = canvas.rgbDataCanvas.height = 1;
 }
  
 canvas.rgbDataContext.fillStyle = 
 'rgb('+[rgb[0]*255,rgb[1]*255,rgb[2]*255].join()+')';
 canvas.rgbDataContext.fillRect(0,0,1,1);
 
 var data = canvas.rgbDataCanvas.toDataURL('image/png','');
  
 return data; 
}

canvas.constantGL = function(c,def)
{ 
 return canvas.gl[c?c:def]; 
}