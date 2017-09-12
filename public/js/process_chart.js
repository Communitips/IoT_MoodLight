var chartData;
var response;
var dataset;

function getFusionData(a_chartData)
{
    var timeArr = new Array()
    var temperArr = new Array()
	var node_number = 2
	temperArr[0] = new Array()
	temperArr[1] = new Array()	

	for(var i = 0; i < a_chartData.length / node_number; i++){
    	timeArr.push({"label":a_chartData[i].Time})
	}
    for(var i = 0; i < a_chartData.length; i++){
      temperArr[a_chartData[i].Node_Number].push({"value":a_chartData[i].Temperature})
    }

    dataset = [
      {
        "seriesname" : "SensorNode 0",
        "data" : temperArr[0],
        "renderAs": "line"
      },{
        "seriesname" : "SensorNode 1",
        "data" : temperArr[1],
        "renderAs": "line"
	  }
    ];

    response = {
      "dataset" : dataset,
      "categories" : timeArr
    };
}

$(function(){


  $.ajax({
    url: '/Temperature/ajax',
    type: 'GET',
    success : function(data) {
      chartData = data;
      getFusionData(chartData)

      var categoriesArray = [{
          "category" : response["categories"]
      }];

      var lineChart = new FusionCharts({
        type: 'mscombidy2d',
                renderAt: 'chart-location',
                width: '800',
                height: '600',
                dataFormat: 'json',
                dataSource: {
                    "chart": {
                        "caption": "Variation of temperature in SensorNode",
                        "subCaption": "2017/08/08",
                        "xAxisname": "시간(hour)",
                        "pYAxisName": "기온 (°C)",
                        "sYAxisMaxValue" : "100",

                        //Cosmetics
                        "paletteColors" : "#0075c2,#1aaf5d,#f2c500",
                        "baseFontColor" : "#333333",
                        "baseFont" : "Helvetica Neue,Arial",
                        "captionFontSize" : "14",
                        "subcaptionFontSize" : "14",
                        "subcaptionFontBold" : "0",
                        "showBorder" : "0",
                        "bgColor" : "#ffffff",
                        "showShadow" : "0",
                        "canvasBgColor" : "#ffffff",
                        "canvasBorderAlpha" : "0",
                        "divlineAlpha" : "100",
                        "divlineColor" : "#999999",
                        "divlineThickness" : "1",
                        "divLineIsDashed" : "1",
                        "divLineDashLen" : "1",
                        "divLineGapLen" : "1",
                        "usePlotGradientColor" : "0",
                        "showplotborder" : "0",
                        "showXAxisLine" : "1",
                        "xAxisLineThickness" : "1",
                        "xAxisLineColor" : "#999999",
                        "showAlternateHGridColor" : "0",
                        "showAlternateVGridColor" : "0",
                        "legendBgAlpha" : "0",
                        "legendBorderAlpha" : "0",
                        "legendShadow" : "0",
                        "legendItemFontSize" : "10",
                        "legendItemFontColor" : "#666666"

                    },
                    "categories" : categoriesArray,
                    "dataset" : response["dataset"]
                  }

      });
      lineChart.render();

    }
  });
});
