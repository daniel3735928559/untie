<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:template match="menuitem">
		<xsl:param name="current" />
		<xsl:param name="expand" />
		<div class="menu_item">
			<a>
				<xsl:attribute name="href"><xsl:value-of select="@url" /></xsl:attribute>
				<xsl:attribute name="class"><xsl:choose><xsl:when test="@url=$current">current</xsl:when><xsl:otherwise>other</xsl:otherwise></xsl:choose></xsl:attribute>
				<xsl:value-of select="@text" />
			</a>
		</div>
		<xsl:if test="(count(./menuitem) > 0 and descendant-or-self::menuitem[@url=$current]) or $expand='always'">
			<div style="padding-left:1.5em;"><xsl:apply-templates select="menuitem"><xsl:with-param name="current" select="$current" /><xsl:with-param name="expand" select="$expand" /></xsl:apply-templates></div>
		</xsl:if>
	</xsl:template>
	<xsl:template match="menu">
		<xsl:param name="current" />
		<xsl:param name="expand" />
		<xsl:param name="heading" />
		<div class="menu"><h3><xsl:value-of select="$heading" />:</h3>
			<xsl:apply-templates select="menuitem"><xsl:with-param name="current" select="$current" /><xsl:with-param name="expand" select="$expand" /></xsl:apply-templates>
		</div>
	</xsl:template>
</xsl:stylesheet>
