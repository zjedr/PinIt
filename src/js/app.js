var timeline = require('timeline');

var PIN_ID = "PinIt-";
console.log('start!');


Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://www.zjed.net/Configuration.html';
  var config = JSON.parse(localStorage.getItem('config'));
  url += '?current=' + encodeURIComponent(JSON.stringify(config));
  console.log('Showing configuration page: ' + url);
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var config = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(config));
  
  LoadConfig(config) ;  
  
});

function LoadConfig(config) {
  
  var message = {0: 1,
      label1: config.label1,
      def1: config.def1,
      inc1: (config.inc1 * 10).toString(),
      label2: config.label2,
      def2: config.def2,
      inc2: (config.inc2 * 10).toString(),
      label3: config.label3,
      def3: config.def3,
      inc3: (config.inc3 * 10).toString(),
      label4: config.label4,
      def4: config.def4,
      inc4: (config.inc4 * 10).toString(),
      label5: config.label5,
      def5: config.def5,
      inc5: (config.inc5 * 10).toString(),
    };
  
  console.log('message: ' + JSON.stringify(message)); 
  Pebble.sendAppMessage(message, function() {
     console.log('Send successful: ' + JSON.stringify(message));
     localStorage.setItem('config', JSON.stringify(config));
  }, function() {
    console.log('Send failed!');
  });
}

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
    Pebble.getTimelineToken(function (token) {

    var internet_status = navigator.onLine;
    console.log("Is the browser online? " + internet_status);

    // tell the C side we're ready
    if (internet_status) {
      Pebble.sendAppMessage({0:0});
      var config = JSON.parse(localStorage.getItem('config'));

      console.log('local:' + JSON.stringify(config));

      if (config !== null ) {
         LoadConfig(config) ;  
      }
    } else {
      Pebble.sendAppMessage({0:2});
    }

    // log the timeline token
    console.log('My timeline token is ' + token);

    // store the token in our global var
//    myToken = token;

  }, function (error) {
    // log the error
    console.log('Error getting timeline token: ' + error);
  });


});

function PostNightscout(s_NS_site, s_type, s_value, s_ns ) {
  
  if (s_type !== '' && s_NS_site !== '' && s_ns !== '' ) {
    var method = 'POST';
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
    };
    
    // Send the request
    request.open(method, url);
    request.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    request.send(params);
  }
}

Pebble.addEventListener('appmessage', function(e) {
  console.log('appmessage read!');

  var s_title = '' ;
  var s_value = 0 ;
  var s_ns = '';
  var s_unit = '';
  var config = JSON.parse(localStorage.getItem('config'));
  
  var reply = e.payload;

  console.log('Got message: ' + JSON.stringify(reply));

  if (reply.label1) {
    s_title = config.label1 ;
    s_ns = config.ns1;
    s_value = reply.label1 ;
  } else if ( reply.label2 ) {
    s_title  = config.label2 ;
    s_ns = config.ns2;
    s_value = reply.label2 ;
  } else if ( reply.label3) {
    s_title  = config.label3 ;
    s_ns = config.ns3;
    s_value = reply.label3 ;
  } else if ( reply.label4 ) {
    s_title  = config.label4 ;
    s_ns = config.ns4;
    s_value = reply.label4 ;
  } else if ( reply.label5 ) {
    s_title  = config.label5 ;
    s_ns = config.ns5;
    s_value = reply.label5 ;
  }
  s_value = s_value / 10 ;
  
  if ( s_ns == 'IOB') { s_unit = 'u';}
  if ( s_ns == 'COB') { s_unit = 'g';}
  console.log( 'title:' +  s_title + ':' + s_value  + ':' + s_ns );
  PostNightscout(config.nightscout_url, s_title, s_value, s_ns);
  if ( console.timeline ) {
    var date = new Date();
  
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
        Pebble.sendAppMessage({0:3});
      });
   } else {
    Pebble.sendAppMessage({0:3});
 }
});

