var timeline = require('timeline');

var PIN_ID = "PinIt-";
	  console.log('start!');

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://www.zjed.net/Configuration.html';
  console.log('Showing configuration page: ' + url);
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));
  
 	var dict = {};
  dict[1] = configData.label1 ;
  dict[2] = configData.default1 ;
  dict[3] = configData.increment1 ;
  console.log('dict: ' + JSON.stringify(dict)); 
  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });

});

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
    Pebble.getTimelineToken(function (token) {

    var internet_status = navigator.onLine;
    console.log("Is the browser online? " + internet_status);

    // tell the C side we're ready
    if (internet_status) {
      Pebble.sendAppMessage({0: true});
    } else {
      Pebble.sendAppMessage({4: true});
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

function PostNightscout(s_type, s_value) {
  
  if (s_type !== '') {
    var method = 'POST';
    var url = 'https://zjedcgm.azurewebsites.net/api/v1/treatments';
    var params = '' ;
    if ( s_type == 'Carbs') {
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
      Pebble.sendAppMessage({5: true});
    };
    
    // Send the request
    request.open(method, url);
    request.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    request.send(params);
  } else {
    Pebble.sendAppMessage({5: true});
  }
}

Pebble.addEventListener('appmessage', function(e) {
  console.log('appmessage read!');

  var s_title = '' ;
  var s_value = 0 ;
  var s_type = '';
  if (e.payload[1]) {
    s_title = 'Insulin:' ;
    s_value = e.payload[1] ;
    s_type = 'Meal+Bolus';
  } else if ( e.payload[2] ) {
    s_title  = 'Carbs:'  ;
    s_value = e.payload[2] ;
    s_type = 'Carbs';
  } else if ( e.payload[3] ) {
    s_title  = 'Protein:'  ;
    s_value = e.payload[3] ;
  }
  if ( e.payload.key == 4 ) { 
        console.log( e.payload[4] );
  } else {
    s_value = s_value / 10 ;
    s_title = s_title + ' ' + s_value ;
    console.log( 'title' +  s_title );
  
    var date = new Date();
  
    // Create the pin
    var pin = {
      "id": "pin-" + PIN_ID + date.toISOString(),
      "time": date.toISOString(),
      "layout": {
        "type": "genericPin",
        "title": s_title,
   //     "body": s_title,
        "tinyIcon": "system://images/GLUCOSE_MONITOR"
      }
      
    };
  
    console.log('Inserting pin: ' + JSON.stringify(pin));
  
    timeline.insertUserPin(pin, function(responseText) { 
      console.log('Result: ' + responseText);
      PostNightscout(s_type,s_value);
    });
  }
   
});

