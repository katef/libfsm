<?xml version="1.0"?>

<xsl:stylesheet version="1.0" 
	xmlns="http://www.w3.org/2000/svg"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:svg="http://www.w3.org/2000/svg"
	xmlns:xlink="http://www.w3.org/1999/xlink"
	xmlns:gv="http://xml.libfsm.org/gv">

<!-- XXX:
	<xsl:param name="dot.inline" select="false()"/>
-->
	<xsl:param name="dot.inline" select="true()"/>

	<!--
		TODO: elide gv: nodes under $dot.inline
		TODO: don't strip colours for inlined dot (except black, etc)
		TODO: use SVG's arrows; see <marker>
	-->

	<xsl:template match="svg:svg">
<xsl:processing-instruction name="xml-stylesheet">href="file:///home/kate/svn/www/var/libfsm-css/style.css" type="text/css"</xsl:processing-instruction>

		<xsl:copy>
			<xsl:apply-templates select="@*"/>

			<xsl:if test="$dot.inline">
				<xsl:attribute name="class">
					<xsl:text>dot</xsl:text>
				</xsl:attribute>
			</xsl:if>

			<xsl:apply-templates select="node()"/>
		</xsl:copy>

		<xsl:if test="not($dot.inline)">
			<xsl:apply-templates select=".//svg:g[@class = 'node']//svg:ellipse" mode="css"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="svg:*">
		<xsl:copy>
			<xsl:apply-templates select="@*|node()"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="svg:a">
		<xsl:apply-templates select="node()"/>
	</xsl:template>

	<xsl:template match="@*">
		<xsl:copy/>
	</xsl:template>

	<xsl:template match="svg:ellipse[    preceding-sibling::svg:ellipse ]"/>
	<xsl:template match="svg:ellipse[not(preceding-sibling::svg:ellipse)]">
		<xsl:if test="@rx != @ry">
			<xsl:message terminate="yes">
				<xsl:text>ellipse rx=</xsl:text>
				<xsl:value-of select="@rx"/>
				<xsl:text> != ry=</xsl:text>
				<xsl:value-of select="@ry"/>
				<xsl:text>&#10;</xsl:text>
			</xsl:message>
		</xsl:if>

		<xsl:variable name="r" select="@rx"/>

		<xsl:choose>
			<xsl:when test="following-sibling::svg:ellipse">
				<circle cx="{@cx}" cy="{@cy}" r="{$r + 0}" class="inner blue"/>
				<circle cx="{@cx}" cy="{@cy}" r="{$r + 3}" class="blue"/>
			</xsl:when>

			<xsl:otherwise>
				<circle cx="{@cx}" cy="{@cy}" r="{$r}" class="blue"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>
<!-- TODO: isn't there a double border? -->

	<xsl:template match="svg:polygon|svg:path">
		<xsl:element name="{name()}">
			<xsl:if test="@stroke = 'white'">
				<xsl:attribute name="class">
					<xsl:text>highlight</xsl:text>
<xsl:text> blue</xsl:text>
				</xsl:attribute>
			</xsl:if>

			<xsl:apply-templates select="@*|node()"/>
		</xsl:element>
	</xsl:template>

	<xsl:template match="svg:text">
		<xsl:variable name="text" select="normalize-space(translate(., ' _', ''))"/>

		<xsl:if test="$text != '' and $text != ' '">
			<xsl:copy>
				<xsl:if test="../svg:polygon/@stroke = 'white'
					or ../svg:ellipse/@stroke = 'white'">
					<xsl:attribute name="class">
						<xsl:text>highlight</xsl:text>
					</xsl:attribute>
				</xsl:if>

				<xsl:apply-templates select="@*|node()|text()"/>
			</xsl:copy>
		</xsl:if>
	</xsl:template>

	<xsl:template match="svg:g">
		<xsl:variable name="from" select="substring-before(svg:title, '-')"/>
		<xsl:variable name="to"   select="substring-after(svg:title, '&gt;')"/>

		<xsl:copy>
			<xsl:if test="not($dot.inline)">
				<xsl:attribute name="gv:edge">
					<xsl:value-of select="concat($from, '-', $to)"/>
				</xsl:attribute>

				<!-- in our .dot, tooltip="1 2 3" specifies the reachable set -->
				<xsl:if test="svg:a/@xlink:title">
					<xsl:attribute name="gv:reachable">
						<xsl:value-of select="svg:a/@xlink:title"/>
					</xsl:attribute>
				</xsl:if>
			</xsl:if>

			<xsl:if test=".//@stroke = 'white'">
				<xsl:attribute name="class">
					<xsl:text>highlight</xsl:text>
				</xsl:attribute>
			</xsl:if>

			<xsl:apply-templates select="@*|node()"/>
		</xsl:copy>
	</xsl:template>

	<!-- TODO: get rid of all $dot.inline; override everything from menu.xsl -->
	<xsl:template match="svg:g[@class = 'node']">
		<xsl:variable name="id" select="svg:title"/>

<!-- TODO: a HTML <a>? -->
<!-- first get the nodes right, then rework the <a>s -->
		<a>
			<xsl:if test="not($dot.inline)">
				<xsl:attribute name="gv:node">
					<xsl:value-of select="$id"/>
				</xsl:attribute>

				<!-- in our .dot, URL="http://..." specifies the link URL, if present -->
				<xsl:if test="svg:a/@xlink:href">
					<xsl:attribute name="xlink:href">
						<xsl:value-of select="svg:a/@xlink:href"/>
					</xsl:attribute>
				</xsl:if>

				<!-- in our .dot, target="/a.*bc?" specifies regexps to match URLs -->
				<xsl:if test="svg:a/@target">
					<xsl:attribute name="gv:match">
						<xsl:value-of select="svg:a/@target"/>
					</xsl:attribute>
				</xsl:if>

				<xsl:if test="count(svg:ellipse) = 2">
					<xsl:attribute name="gv:end">
						<xsl:text>end</xsl:text>
					</xsl:attribute>
				</xsl:if>
			</xsl:if>

			<!-- this node -->
			<xsl:apply-templates select=".//svg:ellipse"/>
			<xsl:apply-templates select=".//svg:text"/>

			<!-- edges from this node -->
			<xsl:apply-templates select="../svg:g[@class = 'edge']
				[substring-before(svg:title, '-') = $id]"/>
		</a>
	</xsl:template>

	<xsl:template match="svg:svg/svg:g">
		<xsl:copy>
			<xsl:apply-templates select="@*"/>

			<xsl:apply-templates select="svg:g[@class = 'node']"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="/svg:svg/@width">
		<xsl:if test="$dot.inline">
			<xsl:copy-of select="."/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="/svg:svg/@height">
		<xsl:if test="$dot.inline">
			<xsl:copy-of select="."/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="/svg:svg/@viewBox">
		<xsl:if test="$dot.inline">
			<xsl:copy-of select="."/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="@fill"/>
	<xsl:template match="@stroke"/>

	<xsl:template match="@font-family"/>
	<xsl:template match="@id"/>
	<xsl:template match="@class"/>
	<xsl:template match="svg:title"/>
	<xsl:template match="svg:path   [@fill = 'none' and @stroke = 'none']"/>
	<xsl:template match="svg:polygon[@fill = 'none' and @stroke = 'none']"/>

</xsl:stylesheet>

