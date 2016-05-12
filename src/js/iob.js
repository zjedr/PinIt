var treatments;  
var latestTreatment = {"date": null  , "value": 0 }; 

function calcIOB(dia) {
  var now = Date.now();
  var treatmentinfo = parseTreatments(now) ;
  treatments = treatmentinfo.treatments ;
  latestTreatment = treatmentinfo.latest ;
  var totalIOB = null ;
  for( var i = 0; i < treatments.length; i++) {
    if (treatments[i].date <= now) {
      var iobContrib = calcTreatment(treatments[i], dia, now);
      if (iobContrib) { totalIOB += iobContrib; }
    }
  }
  var minutesAgo = -1 ;
  if (totalIOB === null ) {totalIOB = -1 ;}
  if (latestTreatment.date !== null ) {
    minutesAgo = ( now  - latestTreatment.date ) / 60000 ;
  }
  console.log ("treatments:" + JSON.stringify(treatments) ) ;
  
  return { IOB: totalIOB, minAgo : parseInt(minutesAgo), lastDose : latestTreatment.value } ;
}

function calcTreatment(treatment, dia, time) {
 
    var scaleFactor = 3.0 / dia, peak = 75, iobContrib = 0;
    iobContrib = 0 ;
    if (treatment.value) {
      var bolusTime = treatment.date;
      var minAgo = scaleFactor * (time - bolusTime) / 1000 / 60;

      if (minAgo < peak) {
        var x1 = minAgo / 5 + 1;
        iobContrib = treatment.value * (1 - 0.001852 * x1 * x1 + 0.001852 * x1);

      } else if (minAgo < 180) {
        var x2 = (minAgo - 75) / 5;
        iobContrib = treatment.value * (0.001323 * x2 * x2 - 0.054233 * x2 + 0.55556);
      }

    }
   console.log ("iob:" + iobContrib ) ;

    return iobContrib;

}

function parseTreatments( now ) {
  // only hols 1 day for iob
	treatments = JSON.parse(localStorage.getItem('treatments'));
  var ONE_DAY = 86400000 ;
  var currentTreatments = [];
  var oldTime = now - ONE_DAY ;
  if ( treatments !== null ) {
    for( var i = 0; i < treatments.length; i++) {
      if (treatments[i].date > latestTreatment.date ) {
        latestTreatment = treatments[i] ;
      }
     if (treatments[i].date > oldTime) {
        currentTreatments.push(treatments[i]);
      }
    }
  }
  localStorage.setItem('treatments', JSON.stringify(currentTreatments));
  
  return { treatments: currentTreatments, latest : latestTreatment } ;
 
}

function addEntry(iobEntry) {
	treatments = JSON.parse(localStorage.getItem('treatments'));
  if ( treatments === null ) {
    treatments = [];
  }
  treatments.push(iobEntry) ;
  localStorage.setItem('treatments', JSON.stringify(treatments));
  console.log ("add:" + JSON.stringify(treatments) ) ;
}

module.exports.calcIOB = calcIOB;
module.exports.addEntry = addEntry;


 // var iobResult = iob.calcTotal(data.treatments, dia);
 