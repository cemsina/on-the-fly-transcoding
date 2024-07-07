# On-the-fly Transcoding (otft)

This project implements a video transcoding microservice using Go and `ffmpeg`. The service supports various endpoints to transcode videos to different formats and resolutions on-the-fly.

## Features

- **On-the-fly Transcoding:** Transcode videos in real-time to different formats and resolutions.
- Transcode videos to MPEG-TS and MP4 formats
- Resize videos to specified dimensions
- Extract specific segments from videos
- Support for video and audio-only segments
- Simple LRU cache to avoid redundant transcoding
- Mutex locking to prevent concurrent transcoding of the same job

## Endpoints

### Transcode Combined Video and Audio Segment

```
GET /{base64_encoded_input_url}/combined/{bitrate}/{width}x{height}/{startAt}:{duration}/segment.{format}
```

- **Description:** Transcodes the given video on-the-fly to the specified format with the given bitrate, dimensions, start time, and duration.
- **Parameters:**
  - `base64_encoded_input_url`: The base64-encoded URL of the input video.
  - `bitrate`: The target bitrate for the output video.
  - `width` and `height`: The target dimensions for the output video.
  - `startAt` and `duration`: The start time and duration of the segment to extract.
  - `format`: The output format, either `ts` or `mp4`.

### Transcode Video Init Segment

```
GET /{base64_encoded_input_url}/video/{bitrate}/{width}x{height}/init.mp4
```

- **Description:** Transcodes the given video on-the-fly to an MP4 init segment with the specified bitrate and dimensions.
- **Parameters:**
  - `base64_encoded_input_url`: The base64-encoded URL of the input video.
  - `bitrate`: The target bitrate for the output video.
  - `width` and `height`: The target dimensions for the output video.

### Transcode Audio Init Segment

```
GET /{base64_encoded_input_url}/audio/{bitrate}/init.mp4
```

- **Description:** Transcodes the given audio on-the-fly to an MP4 init segment with the specified bitrate.
- **Parameters:**
  - `base64_encoded_input_url`: The base64-encoded URL of the input video.
  - `bitrate`: The target bitrate for the output audio.

### Transcode Video Segment

```
GET /{base64_encoded_input_url}/video/{bitrate}/{width}x{height}/{startAt}:{duration}/segment.mp4
```

- **Description:** Transcodes the given video on-the-fly to an MP4 segment with the specified bitrate, dimensions, start time, and duration.
- **Parameters:**
  - `base64_encoded_input_url`: The base64-encoded URL of the input video.
  - `bitrate`: The target bitrate for the output video.
  - `width` and `height`: The target dimensions for the output video.
  - `startAt` and `duration`: The start time and duration of the segment to extract.

### Transcode Audio Segment

```
GET /{base64_encoded_input_url}/audio/{bitrate}/{startAt}:{duration}/segment.mp4
```

- **Description:** Transcodes the given audio on-the-fly to an MP4 segment with the specified bitrate, start time, and duration.
- **Parameters:**
  - `base64_encoded_input_url`: The base64-encoded URL of the input video.
  - `bitrate`: The target bitrate for the output audio.
  - `startAt` and `duration`: The start time and duration of the segment to extract.

## Installation

1. Clone the repository:
```bash
git clone https://github.com/cemsina/on-the-fly-transcoding.git
cd otft
```

2. Setup
```bash
bash setup.sh
```

3. Build the project:
```bash
go build
```

4. Run the server:
```bash
./otft
```


## Environment Variables

- `PORT`: The port on which the server will listen (default: `8080`).

## Example Usage

```bash
ffplay http://localhost:8080/your_base64_encoded_url/combined/1000/1280x720/2.0:5.0/segment.mp4
```

This request transcodes the given video on-the-fly to a 5-second MP4 segment starting from 2 seconds of the video, with a bitrate of 1000kbps and dimensions of 1280x720.

## CDN-friendly URL Architecture
The URL architecture of this microservice is designed to be CDN-friendly, which means it supports efficient content delivery and caching by Content Delivery Networks (CDNs). Here’s how:

- **Unique and Predictable URLs:** Each request URL is uniquely identified by its parameters such as the base64 encoded input URL, bitrate, dimensions, start time, duration, and format. This ensures that identical requests will always generate the same URL, making it easy for CDNs to cache and serve repeated requests without needing to re-fetch or re-transcode the video.

- **Static URLs for Transcoded Content:** The URLs are static and deterministic, meaning the same input parameters will always produce the same output URL. This allows CDNs to cache the transcoded content at the edge, reducing the load on the origin server and improving delivery speed to end-users.

- **Segmented Content Delivery:** By supporting endpoints that deliver specific video or audio segments, the service allows CDNs to cache these segments independently. This is particularly useful for streaming large videos in chunks, as CDNs can store and serve frequently accessed segments more efficiently.

- **Efficient Cache Invalidation:** Since URLs are based on the input parameters, updating any parameter (e.g., changing the bitrate or resolution) will result in a new URL. This helps in efficient cache invalidation, as the CDN will recognize the new URL as distinct content, ensuring users always receive the most up-to-date transcoded content.

- **Scalability:** The CDN-friendly URL structure supports high scalability, enabling the service to handle a large number of concurrent users and requests by leveraging the distributed nature of CDNs.

### Setting Up the Microservice with Google Cloud CDN
To integrate this video transcoding microservice with Google Cloud CDN and configure it to cache the content, follow these steps:

**Prerequisites**
- A Google Cloud Platform (GCP) account.
- Google Cloud SDK installed and configured on your local machine.
- A GCP project set up with billing enabled.

#### Steps to Set Up Google Cloud CDN
1. Deploy the Microservice to Google Cloud Run

First, containerize your application and deploy it to Google Cloud Run.

Create a Dockerfile for your microservice:

```dockerfile
# Use an official Go image to build the binary.
FROM golang:1.16 as builder

WORKDIR /app

# Copy the go module files and download dependencies.
COPY go.mod go.sum ./
RUN go mod download

# Copy the source code.
COPY . .

# Build the Go app.
RUN go build -o otft

# Use a minimal base image.
FROM gcr.io/distroless/base-debian10

COPY --from=builder /app/otft /otft

CMD ["/otft"]
```

Build and push the Docker image to Google Container Registry (GCR):

```bash
docker build -t gcr.io/YOUR_PROJECT_ID/otft .
docker push gcr.io/YOUR_PROJECT_ID/otft
```

Deploy the Docker image to Google Cloud Run:

```bash
gcloud run deploy otft \
    --image gcr.io/YOUR_PROJECT_ID/otft \
    --platform managed \
    --region YOUR_REGION \
    --allow-unauthenticated
```

2. Set Up Google Cloud CDN

Create a backend service and configure it to use the Cloud Run service:
```bash
gcloud compute backend-services create otft-backend \
    --protocol=HTTP --port-name=http --enable-cdn --global

gcloud compute backend-services add-backend otft-backend \
    --balancing-mode=UTILIZATION --max-utilization=0.8 --capacity-scaler=1 \
    --instance-group=YOUR_INSTANCE_GROUP_NAME --global
```

Create a URL map to route incoming requests to your backend service:

```bash
gcloud compute url-maps create otft-url-map \
    --default-service otft-backend
```

Create an SSL certificate (optional, for HTTPS):
```bash
gcloud compute ssl-certificates create otft-ssl-cert \
    --domains=YOUR_DOMAIN
```

Create a target HTTP(S) proxy:
```bash
gcloud compute target-http-proxies create http-proxy \
    --url-map=otft-url-map

gcloud compute target-https-proxies create https-proxy \
    --url-map=otft-url-map --ssl-certificates=otft-ssl-cert
```
```bash
Create a global forwarding rule to route requests to the proxy:

gcloud compute forwarding-rules create http-rule \
    --global --target-http-proxy=http-proxy --ports=80

gcloud compute forwarding-rules create https-rule \
    --global --target-https-proxy=https-proxy --ports=443
```

3. Configure Caching Policies

Set cache expiration rules in your URL map:
```bash
gcloud compute url-maps add-path-matcher otft-url-map \
    --default-service=otft-backend --path-matcher-name=cache-matcher \
    --new-hosts=YOUR_DOMAIN --cache-mode=CACHE_ALL_STATIC \
    --default-max-age=86400
```

Set custom cache keys if needed:
```bash
gcloud compute backend-services update otft-backend \
    --cache-key-policy=includeHost,includeProtocol,includeQueryString
```

4. Verify Your Setup

- Test your setup by sending requests to your domain and checking if the responses are being cached by Cloud CDN.
- Use the Google Cloud Console or the gcloud command-line tool to monitor your CDN configuration and cache hit/miss ratios.

### Example Configuration
Here is an example of how your setup might look in practice:

- Deploy your microservice to Google Cloud Run and note the service URL.
- Set up Google Cloud CDN with the backend service pointing to your Cloud Run service.

By following these steps, you can leverage Google Cloud CDN to efficiently cache and serve the transcoded video files generated by your microservice, providing a scalable and high-performance content delivery solution.