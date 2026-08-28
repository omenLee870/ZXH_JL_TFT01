#ifndef ALI_REALTIME_DEMO_H
#define ALI_REALTIME_DEMO_H

/* Starts continuous audio-only dialogue; server VAD is enabled by default in the demo. */
void ali_realtime_demo_start(void);

/* Starts the legacy SD-card JPEG test flow. */
void ali_realtime_demo_start_sd_image(void);

/* Copies the encoded JPEG before returning; the caller keeps buffer ownership. */
int ali_realtime_demo_start_with_image(const unsigned char *jpeg_data,
                                       unsigned int jpeg_len);

/* Stops a running demo; VAD upload and microphone are released by the demo task. */
void ali_realtime_demo_stop(void);

#endif
