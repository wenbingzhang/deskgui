let hits = 0;
const hitElement = document.querySelector('.hits');

document.body.onkeyup = function (e) {
  if (e.keyCode == 32) {
    addHit();
  }
};

window.webview.onMessage = (message) => {
  console.log(`Message from C++ side: ${message}`);
};

const addHit = () => {
  hits++;
  if (window.counter_value) {
    window.counter_value(hits);
  }
  renderHits();
};

const renderHits = () => {
  hitElement.innerHTML = hits;
};

const resetCounter = () => {
  hits = 0;
  renderHits();
  if (window.counter_value) {
    window.counter_reset(hits);
  }
};

const fetchTimestamp = () => {
  if (window.get_timestamp) {
    const timestampElement = document.getElementById('timestamp');
    if (timestampElement) {
      timestampElement.textContent = 'Loading...';
    }

    window.get_timestamp()
      .then(timestamp => {
        console.log('Manual timestamp fetch:', timestamp);

        if (timestampElement) {
          const date = new Date(parseInt(timestamp));
          timestampElement.textContent = date.toLocaleString();
        }
      })
      .catch(error => {
        console.error('Error getting timestamp:', error);

        if (timestampElement) {
          timestampElement.textContent = 'Error: ' + error.message;
        }
      });
  } else {
    console.error('get_timestamp function not available');
    const timestampElement = document.getElementById('timestamp');
    if (timestampElement) {
      timestampElement.textContent = 'Function not available';
    }
  }
};
