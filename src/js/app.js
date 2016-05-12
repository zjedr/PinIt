var timeline = require('timeline');
var iob = require('iob');

var PIN_ID = "PinIt-";
// multiple floats to pass as integers
var floatMultiplier = 1000;
console.log('start!');


Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://zjedr.github.io/PinIt/Configuration.html';
  var config = JSON.parse(localStorage.getItem('config'));
  url += '?current=' + encodeURIComponent(JSON.stringify(config));
  console.log('Showing configuration page: ' + url);
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var config = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(config));
  localStorage.setItem('config', JSON.stringify(config));
  localStorage.setItem('PinIt_DIA', config.dia);
  
  LoadConfig(config) ;  
  
});

function LoadConfig(config) {
  
  var data = iob.calcIOB(config.dia);
  var mins = data.minAgo ;
  var hours = 0 ;
  var timeString ;
  console.log('data: ' + JSON.stringify(data)); 
  while ( mins >= 60 ) {
     hours+=1 ; 
     mins-=60 ;
   }
  if ( hours > 0 ) {
    timeString = hours.toString() + "h ";
  } else {
    timeString += mins.toString() + "m"; 
  }
  
  var message = {AppKeyReady: 1,
      100: config.label1, 101: config.def1, 102: (config.inc1 * floatMultiplier).toString(), 103: config.ns1,
      200: config.label2, 201: config.def2, 202: (config.inc2 * floatMultiplier).toString(), 203: config.ns2,
      300: config.label3, 301: config.def2, 302: (config.inc3 * floatMultiplier).toString(), 303: config.ns3,
      400: config.label4, 401: config.def2, 402: (config.inc4 * floatMultiplier).toString(), 403: config.ns4,
      500: config.label5, 501: config.def2, 502: (config.inc5 * floatMultiplier).toString(), 503: config.ns5,
      1000: data.IOB.toFixed(2).toString() , 1001: timeString, 1002: parseInt(data.lastDose * floatMultiplier).toString() 
 };
  
  console.log('message: ' + JSON.stringify(message)); 
  Pebble.sendAppMessage(message, function() {
     console.log('Send successful: ' + JSON.stringify(message));
   //  console.log('save config: ' + JSON.stringify(config));
     localStorage.setItem('config', JSON.stringify(config));
  }, function() {
    console.log('Send failed!');
  });
}

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
 
    var internet_status = navigator.onLine;
    console.log("Is the browser online? " + internet_status);

    // tell the C side we're ready
    Pebble.sendAppMessage({0:0});
    var config = JSON.parse(localStorage.getItem('config'));
  //  var config = null ;
 
    if (config === null ) {
        config = {"nightscout_url":"","timeline":true,"label1":"Bolus","def1":"1","inc1":"0.5","ns1":"IOB","label2":"Carbs","def2":"10","inc2":"0.25","ns2":"","label3":"test","def3":"1","inc3":"0.25","ns3":"IOB","label4":"","def4":"","inc4":"","ns4":"","label5":"","def5":"","inc5":"","ns5":"","dia":"3"} ;
    }

    console.log('local:' + JSON.stringify(config));
    LoadConfig(config) ;  

});

function PostNightscout(s_NS_site, s_type, s_value, s_ns ) {
  
  if (s_type !== '' && s_NS_site !== '' && s_ns !== '' ) {
    var url = s_NS_site + '/api/v1/treatments';
    var params = '' ;
    if ( s_ns == 'COB') {
      params = 'eventType=' + s_type + '&carbs=' + s_value;
    } else {
      params = 'eventType=' + s_type + '&insulin=' + s_value;
    }
       console.log('url: ' + url + '?' + params );
    
    // Create the request
    var request = new XMLHttpRequest();
    
    // Specify the callback for when the request is completed
    request.onload = function() {
      // The request was successfully completed!
      console.log('Got response: ' + this.responseText);
      if ( request.status != 200 ) {
        Pebble.sendAppMessage({AppKeyReady:2,AppKeyMessage:"Nightscout request Failed."});
      }
    };
    
    // Send the request
    request.open('POST', url);
    request.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    request.send(params);
  } else {
           Pebble.sendAppMessage({AppKeyReady:2,AppKeyMessage:"Nightscout request Failed."});
 
  }
}

Pebble.addEventListener('appmessage', function(e) {
  console.log('appmessage read!');

  var s_title = '' ;
  var s_value = 0 ;
  var s_ns = '';
  var s_unit = '';
  var config = JSON.parse(localStorage.getItem('config'));
  var date = new Date();
 
  var reply = e.payload;

  console.log('Got message: ' + JSON.stringify(reply));

  if (reply.AppKeyTreatment1) {
    s_title = config.label1 ;
    s_ns = config.ns1;
    s_value = reply.AppKeyTreatment1 ;
  } else if ( reply.AppKeyTreatment2 ) {
    s_title  = config.label2 ;
    s_ns = config.ns2;
    s_value = reply.AppKeyTreatment2 ;
  } else if ( reply.AppKeyTreatment3) {
    s_title  = config.label3 ;
    s_ns = config.ns3;
    s_value = reply.AppKeyTreatment3 ;
  } else if ( reply.AppKeyTreatment4 ) {
    s_title  = config.label4 ;
    s_ns = config.ns4;
    s_value = reply.AppKeyTreatment4 ;
  } else if ( reply.AppKeyTreatment5 ) {
    s_title  = config.label5 ;
    s_ns = config.ns5;
    s_value = reply.AppKeyTreatment5 ;
  }
  s_value = s_value / floatMultiplier ;
  
  if ( s_ns == 'IOB') { 
    s_unit = 'u';
    var iobEntry = {"date": Date.now()  , "value": parseFloat(s_value) }; 
    console.log("entry:" + JSON.stringify(iobEntry)) ;
    iob.addEntry(iobEntry) ;    
  }
  if ( s_ns == 'COB') { s_unit = 'g';}
  
  console.log( 'title:' +  s_title + ':' + s_value  + ':' + s_ns );
  PostNightscout(config.nightscout_url, s_title, s_value, s_ns);
  if ( console.timeline ) {
  
    // Create the pin
    var pin = {
      "id": "pin-" + PIN_ID + date.toISOString(),
      "time": date.toISOString(),
      "layout": {
        "type": "genericPin",
        "title": s_title + ' ' + s_value + s_unit,
        "tinyIcon": "system://images/GLUCOSE_MONITOR"
      }
    };
    
    console.log('Inserting pin: ' + JSON.stringify(pin));
    
    timeline.insertUserPin(pin, function(responseText) { 
      console.log('Result: ' + responseText);
      Pebble.sendAppMessage({AppKeyReady:3});
    });
   } else {
    Pebble.sendAppMessage({AppKeyReady:3});
 }
});

