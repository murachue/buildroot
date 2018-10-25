#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <tremor/ivorbisfile.h>
#include <alsa/asoundlib.h>

#define DEBUG

static snd_pcm_t *pcm;
static volatile sig_atomic_t aborting = 0;

static void sighandler(int sig) {
	if(aborting) {
		return;
	}

	aborting = 1;
	write(1, "abort\n", 6); // avoid calling printf in signal handler!
	if(pcm) {
		snd_pcm_abort(pcm); // = snd_pcm_nonblock(pcm, 2); TODO can it be called in signal handler??
	}
	if(sig == SIGABRT) {
		/* do not call snd_pcm_close() and abort immediately */
		pcm = NULL;
		exit(EXIT_FAILURE);
	}
	signal(sig, SIG_DFL);
}

int main(int argc, char **argv) {
	const char * const argv0 = argv[0];
	char *dev = "default";
	int buffer_time = 0, period_time = 0;
	int verbose = 0;
	char *fname = NULL;
	int ret;

	{
		int opt;
		while((opt = getopt(argc, argv, "d:b:p:v")) != -1) {
			switch(opt) {
			case 'd':
				dev = optarg;
				break;
			case 'b':
				buffer_time = atoi(optarg);
				break;
			case 'p':
				period_time = atoi(optarg);
				break;
			case 'v':
				verbose++;
				break;
			default:
				fprintf(stderr, "Unknown option: %c?\nUsage: %s [-d dev] ogg_file\n", opt, argv0);
				exit(EXIT_FAILURE);
			}
		}
		argc -= optind;
		argv += optind;

		fname = argv[0];
		argc -= 1;
		argv += 1;

		if(!fname) {
			fprintf(stderr, "Please specify a ogg file.\n");
			exit(EXIT_FAILURE);
		}
	}

	{
		FILE *fp;
		OggVorbis_File vf;
		vorbis_info *vi;

		if((fp = fopen(fname, "rb")) == NULL) {
			perror("could not open file");
			exit(EXIT_FAILURE);
		}

		if((ret = ov_open(fp, &vf, /*no initial*/NULL, 0)) < 0) {
			fprintf(stderr, "could not open oggvorbis: %d\n", ret);
			exit(EXIT_FAILURE);
		}

		vi = ov_info(&vf, -1/*current link*/);
		printf("ov_info: %dch %ldHz\n", vi->channels, vi->rate);

		{
			snd_output_t *log;
			snd_pcm_uframes_t period_size, buffer_size;
			void *buf;
			size_t bufsize, bytesperframe;

			/* for logging by alsa-lib */
			if((ret = snd_output_stdio_attach(&log, stderr, 0)) < 0) {
				fprintf(stderr, "snd_output_stdio_attach error=%d\n", ret);
				exit(EXIT_FAILURE);
			}
			if((ret = snd_pcm_open(&pcm, dev, SND_PCM_STREAM_PLAYBACK, 0/*no flags*/)) < 0) {
				fprintf(stderr, "snd_pcm_open error=%d (broken configuration?)\n", ret);
				exit(EXIT_FAILURE);
			}

			/* setting handler here, or totally ignored?? */
			signal(SIGINT, sighandler);
			signal(SIGTERM, sighandler);
			signal(SIGABRT, sighandler);

			{
				snd_pcm_hw_params_t *hwparams;
				snd_pcm_hw_params_alloca(&hwparams);
				if((ret = snd_pcm_hw_params_any(pcm, hwparams)) < 0) {
					fprintf(stderr, "snd_pcm_hw_params_any error=%ld\n", ret);
					exit(EXIT_FAILURE);
				}
				if(verbose) {
					fprintf(stderr, "default hwparams of \"%s\":\n", snd_pcm_name(pcm));
					snd_pcm_hw_params_dump(hwparams, log);
				}
				/* TODO support mmap? SND_PCM_ACCESS_MMAP_INTERLEAVED */
				if((ret = snd_pcm_hw_params_set_access(pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
					fprintf(stderr, "snd_pcm_hw_params_set_access error=%d (access type not available)\n", ret);
					exit(EXIT_FAILURE);
				}
				if((ret = snd_pcm_hw_params_set_format(pcm, hwparams, SND_PCM_FORMAT_S16/*tremor_s16 native*/)) < 0) {
					fprintf(stderr, "snd_pcm_hw_params_set_format error=%d (sample format not available)\n", ret);
					exit(EXIT_FAILURE);
				}
				if((ret = snd_pcm_hw_params_set_channels(pcm, hwparams, vi->channels)) < 0) {
					fprintf(stderr, "snd_pcm_hw_params_set_format error=%d (sample format not available)\n", ret);
					exit(EXIT_FAILURE);
				}
				{
					int rate = vi->rate;
					if((ret = snd_pcm_hw_params_set_rate_near(pcm, hwparams, &rate, 0)) < 0) {
						fprintf(stderr, "snd_pcm_hw_params_set_rate_near error=%d\n", ret);
						exit(EXIT_FAILURE);
					}
					printf("rate %d(vorbis) -> %d(hw)\n", vi->rate, rate);
				}
				/* default buffer_time and period_time */
				{
					if(buffer_time == 0) {
						if((ret = snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0)) < 0) {
							fprintf(stderr, "snd_pcm_hw_params_get_buffer_time_max error=%d\n", ret);
							exit(EXIT_FAILURE);
						}
					}
					if(period_time == 0) {
						period_time = buffer_time / 4;
					}
				}
				if((ret = snd_pcm_hw_params_set_period_time_near(pcm, hwparams, &period_time, 0)) < 0) {
					fprintf(stderr, "snd_pcm_hw_params_set_period_time_near error=%d\n", ret);
					exit(EXIT_FAILURE);
				}
				if((ret = snd_pcm_hw_params_set_buffer_time_near(pcm, hwparams, &buffer_time, 0)) < 0) {
					fprintf(stderr, "snd_pcm_hw_params_set_buffer_time_near error=%d\n", ret);
					exit(EXIT_FAILURE);
				}
				if((ret = snd_pcm_hw_params(pcm, hwparams)) < 0) {
					fprintf(stderr, "snd_pcm_hw_params error=%d\n", ret);
					snd_pcm_hw_params_dump(hwparams, log);
					exit(EXIT_FAILURE);
				}
				snd_pcm_hw_params_get_period_size(hwparams, &period_size, 0);
				snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
				printf("frames: period=%d buffer=%d\n", period_size, buffer_size);
			}

			{
				snd_pcm_sw_params_t *swparams;
				snd_pcm_sw_params_alloca(&swparams);
				snd_pcm_sw_params_current(pcm, swparams);
				/* wakeup threshold */
				if((ret = snd_pcm_sw_params_set_avail_min(pcm, swparams, period_size/*uh.*/)) < 0) {
					fprintf(stderr, "snd_pcm_sw_params_set_avail_min error=%d\n", ret);
					exit(EXIT_FAILURE);
				}
				/* auto-start threshold */
				if((ret = snd_pcm_sw_params_set_start_threshold(pcm, swparams, buffer_size/*uh...?? TODO <bufsize cause XRUN */)) < 0) {
					fprintf(stderr, "snd_pcm_sw_params_set_avail_min error=%d\n", ret);
					exit(EXIT_FAILURE);
				}
				/* auto-stop on underrun when avail_frames >= threshold ...?? TODO <bufsize cause XRUN on full-buffer write?? */
				if((ret = snd_pcm_sw_params_set_stop_threshold(pcm, swparams, buffer_size/*uh...??*/)) < 0) {
					fprintf(stderr, "snd_pcm_sw_params_set_stop_threshold error=%d\n", ret);
					exit(EXIT_FAILURE);
				}
				if((ret = snd_pcm_sw_params(pcm, swparams)) < 0) {
					fprintf(stderr, "snd_pcm_sw_params error=%d\n", ret);
					snd_pcm_sw_params_dump(swparams, log);
					exit(EXIT_FAILURE);
				}
			}

			if(verbose) {
				snd_pcm_dump(pcm, log);
			}

			bytesperframe = snd_pcm_format_physical_width(SND_PCM_FORMAT_S16_BE) / 8 * vi->channels;
			bufsize = period_size * bytesperframe;
			if((buf = malloc(bufsize)) == NULL) {
				fprintf(stderr, "ENOMEM for buffer\n");
				exit(EXIT_FAILURE);
			}

			for(;;) {
				//int bitstream;
				long r = ov_read(&vf, buf, bufsize, /*&bitstream*/NULL);
#ifdef DEBUG
				printf("ov_read=%4ld (%4ld) ", r, r / bytesperframe);
#endif
				if(r < 0) {
					/* error */
					fprintf(stderr, "ov_read error=%ld\n", r);
					break;
				} else if(r == 0) {
					/* EOF */
					break;
				}
				{
					snd_pcm_uframes_t rf = r / bytesperframe;
					char *p = buf;
					while(0 < rf) {
						/* snd_pcm_avail_delay(pcm, &sframest_avail, &sframest_delay) */
						/* TODO mmap */
#ifdef DEBUG
						fflush(stdout);
#endif
						snd_pcm_sframes_t wf = snd_pcm_writei(pcm, p, rf);
#ifdef DEBUG
						{
							snd_pcm_sframes_t avail, delay;
							snd_pcm_avail_delay(pcm, &avail, &delay);
							printf("snd_pcm_writei=%4d a=%5d d=%5d %s\n", wf, avail, delay, snd_pcm_state_name(snd_pcm_state(pcm)));
						}
#endif
						if(wf == rf) {
							break;
						} else if(wf == -EAGAIN || (0 <= wf)) {
							if(0 <= wf) {
								rf -= wf;
								p += wf * bytesperframe;
							}
#ifdef DEBUG
							printf("snd_pcm_wait\n");
#endif
							snd_pcm_wait(pcm, 100); /* wait, 100ms timeout */
						} else if(wf == -EPIPE) {
							snd_pcm_status_t *status;
							snd_pcm_state_t state;

							snd_pcm_status_alloca(&status);
							if((wf = snd_pcm_status(pcm, status)) < 0) {
								fprintf(stderr, "snd_pcm_status error=%d\n", wf);
								exit(EXIT_FAILURE);
							}
							state = snd_pcm_status_get_state(status);
							if(state == SND_PCM_STATE_XRUN) {
								/* TODO diff snd_pcm_status_get_trigger_htstamp and clock_gettime(CLOCK_MONOTONIC) to show at-least stop time? */
								fprintf(stderr, "XRUN\n");
								if(verbose) {
									fprintf(stderr, "status:\n");
									snd_pcm_status_dump(status, log);
								}
								if((wf = snd_pcm_prepare(pcm)) < 0) {
									fprintf(stderr, "snd_pcm_prepare error=%d\n", wf);
									exit(EXIT_FAILURE);
								}
#ifdef DEBUG
								//goto bye;
#endif
							} else {
								fprintf(stderr, "EPIPE but %s\n", snd_pcm_state_name(state));
								if(verbose) {
									fprintf(stderr, "status:\n");
									snd_pcm_status_dump(status, log);
								}
								exit(EXIT_FAILURE);
							}
						} else if(wf == -ESTRPIPE) {
							if(verbose) {
								fprintf(stderr, "ESTRPIPE\n");
							}
							while((wf = snd_pcm_resume(pcm)) == -EAGAIN) {
								sleep(1);	/* wait until suspend flag is released */
							}
							if (wf < 0) {
								fprintf(stderr, "snd_pcm_resume failed, restarting stream.\n");
								if((wf = snd_pcm_prepare(pcm)) < 0) {
									fprintf(stderr, "snd_pcm_prepare error=%d\n", wf);
									exit(EXIT_FAILURE);
								}
							}
							fprintf(stderr, "recovered from ESTRPIPE\n");
						} else {
							fprintf(stderr, "pcm_write error=%d\n", r);
							goto bye;
						}
					}
				}
			}
bye:

			snd_pcm_drain(pcm);

			snd_pcm_close(pcm);
			snd_output_close(log);
			snd_config_update_free_global(); // what is this???
		}

		ov_clear(&vf);
	}

	printf("Done.\n");
	exit(EXIT_SUCCESS);
}
