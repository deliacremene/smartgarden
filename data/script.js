  // sensor data
  // to automatically update those values every 10 seconds without the need for refreshing the page
  // make a request in the "/temperature" URL
  // to get the last soil temperature value read we update the element with the id="temperature"
  // => the temperature is updated asynchronously
  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("temperature").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/temperature", true);
    xhttp.send();
  }, 10000 ) ;

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("humidity").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/humidity", true);
    xhttp.send();
  }, 10000 ) ;

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("moisture").innerHTML = this.responseText;
        //convert int and set img
      }
    };
    xhttp.open("GET", "/moisture", true);
    xhttp.send();
  }, 10000 ) ;

  // weather data
  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("owaweather").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/owaweather", true);
    xhttp.send();
  }, 10000 ) ;

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("owadescription").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/owadescription", true);
    xhttp.send();
  }, 10000 ) ;

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("owatemperature").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/owatemperature", true);
    xhttp.send();
  }, 10000 ) ;

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("owapressure").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/owapressure", true);
    xhttp.send();
  }, 10000 ) ;

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("owahumidity").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/owahumidity", true);
    xhttp.send();
  }, 10000 ) ;

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("owawindspeed").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/owawindspeed", true);
    xhttp.send();
  }, 10000 ) ;

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("owaicon").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/owaicon", true);
    xhttp.send();
  }, 10000 ) ;

  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("flower").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/flower", true);
    xhttp.send();
  }, 10000 ) ;


//firebase project configuration
const config = {
  apiKey: "AIzaSyCrMwLHAT6qPePZD3UBzJl4Yc-pSOBhU_c",
  authDomain: "smartirrigation-6cf2c.firebaseapp.com",
  databaseURL: "https://smartirrigation-6cf2c-default-rtdb.firebaseio.com",
  projectId: "smartirrigation-6cf2c",
  storageBucket: "smartirrigation-6cf2c.appspot.com",
  messagingSenderId: "317885064937",
  };

firebase.initializeApp(config);

// number of last elements to work with in the 'timestamped_measures'
const no_elements = 800;

// each time a new value is added in the 'timestamped_measures' node
// we make arrays of the last 'no_elements' soil_moisture_values, air_humidity_values, air_temperature_values for the x axis 
// and timestamps for the y axix
// firebase trigger syntax
firebase.database().ref('timestamped_measures').limitToLast(no_elements).on('value', timestamped_measures => {
    // 'ts_measures' is obtained on changed value of 'timestamped_measures' node
    // we need to apply the val() method to it
    // ts_measures.val()
    // => {-Mc8uy553ugChFvJu-KW: {air_humidity: 58.3 , air_temperature: 22.3, soil_moisture: 53, timestamp: 1623664090}, -Mc8v-XMYdq6vfbAZvhZ: {…}, … }

    let timestamps = [];
    let soil_moisture_values = [];
    let air_humidity_values = [];
    let air_temperature_values = [];


    // we iterate on every element of the 'timestamped_measures' in order to fill the arrays with 'no_elements' values
    // we apply the val() method to access the values of 'soil_moisture', 'air_humidity', 'air_temperature' and 'timestamp'
    // and push the last 'no_elements' to the corresponding arrays
    // we also convert the Epoch time to real date  
    timestamped_measures.forEach(e => {
        soil_moisture_values.push(e.val().soil_moisture);
        air_humidity_values.push(e.val().air_humidity);
        air_temperature_values.push(e.val().air_temperature);
        timestamps.push(new Date(e.val().timestamp*1000));
    });

    plot_div_1 = document.getElementById('plot_1');
    plot_div_2 = document.getElementById('plot_2');
    plot_div_3 = document.getElementById('plot_3');



    // we give the x and y data to plotly.js to draw the plot and also the layout information
    const data1 = [{
        x: timestamps,
        y: soil_moisture_values
    }];

    const layout1 = {
        title: '<b>Soil moisture live plot</b>',
        titlefont: {
            family: 'Courier New, monospace',
            size: 18
        },
        xaxis: {
            title: '<b>Time</b>',
            titlefont: {
              family: 'Courier New, monospace',
              size: 16
            },
            linecolor: 'black',
            linewidth: 2
        },
        yaxis: {
            title: '<b>Soil moisture %</b>',
            titlefont: {
                family: 'Courier New, monospace',
                size: 16
            },
            linecolor: 'black',
            linewidth: 2,
            range: [0, 100]
        },
        margin: {
            r: 50,
            pad: 0
        }
    }
    Plotly.newPlot(plot_div_1, data1, layout1, { responsive: true });


    const data2 = [{
        x: timestamps,
        y: air_temperature_values
    }];

    const layout2 = {
        title: '<b>Air temperature live plot</b>',
        titlefont: {
            family: 'Courier New, monospace',
            size: 18
        },
        xaxis: {
            title: '<b>Time</b>',
            titlefont: {
              family: 'Courier New, monospace',
              size: 16
            },
            linecolor: 'black',
            linewidth: 2,
        },
        yaxis: {
            title: '<b>Air temperature &#8451;</b>',
            titlefont: {
                family: 'Courier New, monospace',
                size: 16
            },
            linecolor: 'black',
            linewidth: 2,
            range: [10, 30]
        },
        margin: {
            r: 50,
            pad: 0
        }
    }
    Plotly.newPlot(plot_div_2, data2, layout2, { responsive: true });

    
    const data3 = [{
        x: timestamps,
        y: air_humidity_values
    }];

    const layout3 = {
        title: '<b>Air humidity live plot</b>',
        titlefont: {
            family: 'Courier New, monospace',
            size: 18
        },
        xaxis: {
            title: '<b>Time</b>',
            titlefont: {
              family: 'Courier New, monospace',
              size: 16
            },
            linecolor: 'black',
            linewidth: 2
        },
        yaxis: {
            title: '<b>Air humidity %</b>',
            titlefont: {
                family: 'Courier New, monospace',
                size: 16
            },
            linecolor: 'black',
            linewidth: 2,
            range: [0, 100]
        },
        margin: {
            r: 50,
            pad: 0
        }
    }
    Plotly.newPlot(plot_div_3, data3, layout3, { responsive: true });

});


