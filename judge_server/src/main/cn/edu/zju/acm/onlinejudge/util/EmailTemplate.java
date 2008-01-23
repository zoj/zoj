package cn.edu.zju.acm.onlinejudge.util;

import java.util.Properties;

public class EmailTemplate {
	private final String title;
	private final String replayTo;
	private final String content;
	
	public EmailTemplate(String title, String replyTo, String content) {
		this.title = title;
		this.replayTo = replyTo;
		this.content = content;
	}

	public String getTitle() {
		return title;
	}

	public String getReplayTo() {
		return replayTo;
	}

	public String getContent() {
		return content;
	}
	
	public String getTitle(Properties properties) {
		return render(title, properties);
	}

	public String getReplayTo(Properties properties) {
		return render(replayTo, properties);
	}

	public String getContent(Properties properties) {
		return render(content, properties);
	}
	
	public String render(String text, Properties properties) {
		StringBuilder sb = new StringBuilder();
		int k = 0;
		for (;;) {
			int i = text.indexOf('$', k);
			if (i == -1) {
				break;
			}
			int j = text.indexOf('$', i + 1);
			if (j == -1) {
				break;
			}
			sb.append(text.substring(k, i));
			if (i + 1 == j) {
				sb.append('$');
			} else {
				String key = text.substring(i+1, j);
				if (properties.containsKey(key)) {
					sb.append(properties.getProperty(key));
				} else {
					sb.append('$').append(key).append('$');
				}
			}
			k = j + 1;
		}
		sb.append(text.substring(k));
		return sb.toString();
	}
}
