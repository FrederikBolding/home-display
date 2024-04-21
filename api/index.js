const express = require("express");
const app = express();
require("dotenv").config();

const {
  HOME_ASSISTANT_API,
  HOME_ASSISTANT_TOKEN,
  CALENDAR_ID,
  TEMP_SENSORS,
  LONGITUDE,
  LATITUDE,
} = process.env;

async function fetchJson(url, headers = {}) {
  const response = await fetch(url, {
    headers,
  });

  return await response.json();
}

async function fetchHomeAssistant(path) {
  return fetchJson(`${HOME_ASSISTANT_API}/${path}`, {
    authorization: HOME_ASSISTANT_TOKEN,
  });
}

async function fetchWeather() {
  const weather = await fetchJson(
    `https://api.open-meteo.com/v1/dwd-icon?latitude=${LATITUDE}&longitude=${LONGITUDE}&current=temperature_2m&timezone=auto`
  );

  const temperature =
    Math.round(weather.current["temperature_2m"]) +
    weather.current_units["temperature_2m"];

  return { temperature };
}

async function fetchCalendarEvents() {
  const start = new Date();
  const end = new Date(start.getTime() + 86400000 * 14);
  const events = await fetchHomeAssistant(
    `calendars/${CALENDAR_ID}?start=${start
      .toISOString()
      .substring(0, 10)}&end=${end.toISOString().substring(0, 10)}`
  );

  return events
    .map((event) => {
      const startDate = event.start.date ?? event.start.dateTime;
      const endDate = event.end.date ?? event.end.dateTime;
      return { title: event.summary, start: startDate, end: endDate };
    })
    .sort((a, b) => new Date(a.start) - new Date(b.start));
}

async function fetchEntityState(id) {
  return fetchHomeAssistant(`states/${id}`);
}

async function fetchTemperatures() {
  const entityStates = await Promise.all(
    TEMP_SENSORS.split(",").map((id) => fetchEntityState(id))
  );

  return entityStates.map((entityState) => ({
    name: entityState.attributes.friendly_name,
    value:
      parseFloat(entityState.state, 10).toFixed(1) +
      entityState.attributes.unit_of_measurement,
  }));
}

async function requestHandler(request, response) {
  try {
    const weather = await fetchWeather();

    const calendarEvents = await fetchCalendarEvents();

    const temperatures = await fetchTemperatures();

    response.send({ weather, calendarEvents, temperatures });
  } catch (error) {
    console.error(error);
    response.status(500).send("Internal Server Error");
  }
}

app.get("/", requestHandler);

app.listen(3000);

console.log("Listening on :3000");
