# Ticket Practice Flow Design

## Goal

Add a realistic three-page mobile ticket practice flow before the existing capacitive touch test.

## Pages

- `/test/`: event detail page with a 10-second countdown and a persistent red "已预约" button.
- `/test/reserve/`: reservation page for selecting a date and price.
- `/test/run/`: the existing capacitive touch interval test.

## Routing Rules

The detail page stores an absolute target timestamp in `sessionStorage`, so the countdown continues while visiting other pages.

- Clicking "已预约" with more than 3 seconds remaining opens `/test/reserve/`.
- Clicking "已预约" with 3 seconds or less remaining opens `/test/run/`.
- Clicking after the countdown reaches zero also opens `/test/run/`.
- "重新练习" resets the target to 10 seconds from the current time.

## Visual Direction

The detail and reservation pages imitate the supplied mobile ticketing screenshots: warm red promotional header, white rounded content sheets, dense event information, and a fixed bottom action area. The pages use original CSS shapes and text rather than reproducing third-party brand assets.

## Testing

Unit tests cover countdown initialization, persisted targets, remaining-time calculation, reset behavior, and the 3-second routing boundary. Browser verification covers all three pages and both routing branches.
